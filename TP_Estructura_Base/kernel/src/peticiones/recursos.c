#include <peticiones/recursos.h>
#include <commons/log.h>
#include <string.h>
#include <stdlib.h>
#include <commons/string.h>

#include <mod_kernel.h>
// extern t_log* logger;
static t_dictionary* diccionario_recursos = NULL;

void recursos_init(t_kernel_config* config) {
    diccionario_recursos = dictionary_create();
    
    char** nombres = config->recursos;
    char** instancias = config->instancias_recursos;
    
    if(!nombres || !instancias) return;

    int i = 0;
    while(nombres[i] != NULL) {
        t_recurso* rec = malloc(sizeof(t_recurso));
        rec->nombre = strdup(nombres[i]);
        rec->instancias = atoi(instancias[i]);
        rec->cola_bloqueados = list_create();
        pthread_mutex_init(&rec->mutex, NULL);
        
        dictionary_put(diccionario_recursos, rec->nombre, rec);
        
        log_info(KERNEL_CTX.logger, "Recurso: %s inicializado instancias=%d", rec->nombre, rec->instancias);
        i++;
    }
}

static void destruir_recurso(t_recurso* r) {
    if(!r) return;
    free(r->nombre);
    list_destroy(r->cola_bloqueados); // PCBs should be handled by kernel destroy logic or explicit removal
    pthread_mutex_destroy(&r->mutex);
    free(r);
}

void recursos_destroy() {
    if(diccionario_recursos) {
        dictionary_destroy_and_destroy_elements(diccionario_recursos, (void*)destruir_recurso);
    }
}

bool recurso_wait(t_pcb* pcb, char* nombre_recurso) {
    if (!dictionary_has_key(diccionario_recursos, nombre_recurso)) {
        log_error(KERNEL_CTX.logger, "PID: %d - WAIT error: recurso inexistente %s", pcb->pid, nombre_recurso);
        // Treat as no-block but maybe abort process? For now, no block.
        return false; 
    }

    t_recurso* r = dictionary_get(diccionario_recursos, nombre_recurso);
    
    pthread_mutex_lock(&r->mutex);
    if (r->instancias > 0) {
        r->instancias--;
        list_add(pcb->recursos_adquiridos, strdup(nombre_recurso));
        log_info(KERNEL_CTX.logger, "PID: %d - WAIT recurso %s asignado, instancias=%d", pcb->pid, r->nombre, r->instancias);
        pthread_mutex_unlock(&r->mutex);
        return false; // No bloquear
    } else {
        log_info(KERNEL_CTX.logger, "PID: %d - WAIT recurso %s ocupado, bloqueado", pcb->pid, r->nombre);
        list_add(r->cola_bloqueados, pcb); // Add pointer. Ownership shared? Usually owned by Blocked Queue globally.
        // Wait, if we put it here, do we ALSO put it in global blocked queue? 
        // Yes, Kernel puts it in global Blocked to manage state, but resource keeps ref to unlock it specifically.
        pthread_mutex_unlock(&r->mutex);
        return true; // Bloquear
    }
}

t_pcb* recurso_signal(char* nombre_recurso, t_pcb* pcb_signaler) {
    if (!dictionary_has_key(diccionario_recursos, nombre_recurso)) {
        log_error(KERNEL_CTX.logger, "SIGNAL error: recurso inexistente %s", nombre_recurso);
        return NULL;
    }

    t_recurso* r = dictionary_get(diccionario_recursos, nombre_recurso);
    t_pcb* desbloqueado = NULL;

    // Remover recurso de la lista del proceso que hace signal
    if (pcb_signaler && pcb_signaler->recursos_adquiridos) {
        for (int i = 0; i < list_size(pcb_signaler->recursos_adquiridos); i++) {
            char* res = list_get(pcb_signaler->recursos_adquiridos, i);
            if (strcmp(res, nombre_recurso) == 0) {
                list_remove(pcb_signaler->recursos_adquiridos, i);
                free(res);
                break;
            }
        }
    }

    pthread_mutex_lock(&r->mutex);

    if (list_is_empty(r->cola_bloqueados)) {
        r->instancias++;
        log_info(KERNEL_CTX.logger, "SIGNAL: recurso %s liberado, instancias=%d", r->nombre, r->instancias);
    } else {
        desbloqueado = list_remove(r->cola_bloqueados, 0);
        // El recurso se transfiere al desbloqueado
        list_add(desbloqueado->recursos_adquiridos, strdup(nombre_recurso));
        log_info(KERNEL_CTX.logger, "PID: %d - SIGNAL recurso %s asignado (desbloqueado)", desbloqueado->pid, r->nombre);
    }

    pthread_mutex_unlock(&r->mutex);
    return desbloqueado;
}

t_dictionary* recursos_obtener_diccionario(void) {
    return diccionario_recursos;
}

void recursos_liberar_proceso(uint32_t pid) {
    if (!diccionario_recursos) return;
    
    t_list* resource_names = dictionary_keys(diccionario_recursos);
    
    for(int i=0; i<list_size(resource_names); i++) {
        char* nombre = list_get(resource_names, i);
        t_recurso* r = dictionary_get(diccionario_recursos, nombre);
        
        if (r) {
            pthread_mutex_lock(&r->mutex);
            
            // Remove from blocked queue if present
            for(int j=0; j<list_size(r->cola_bloqueados); j++) {
                t_pcb* pcb = list_get(r->cola_bloqueados, j);
                if (pcb && pcb->pid == pid) {
                    list_remove(r->cola_bloqueados, j);
                    log_info(KERNEL_CTX.logger, "PID: %d - Removido de cola bloqueados recurso %s", pid, nombre);
                    break;
                }
            }
            
            pthread_mutex_unlock(&r->mutex);
        }
    }
    
    list_destroy(resource_names);
}
