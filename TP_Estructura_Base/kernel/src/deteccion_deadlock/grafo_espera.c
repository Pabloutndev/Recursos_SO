#include <deteccion_deadlock/grafo_espera.h>
#include <peticiones/recursos.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/logger.h>
#include <mod_kernel.h>

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>
#include <stdlib.h>

// Nodo del grafo: un proceso con su lista de aristas (PIDs a los que espera)
typedef struct {
    uint32_t pid;
    t_pcb*   pcb;
    char*    recurso_esperado; // nombre del recurso que espera (NULL si no esta bloqueado)
    t_list*  espera_a;         // lista de uint32_t* (PIDs de procesos que tienen el recurso)
} t_nodo_grafo;

// Buscar que recurso esta esperando un proceso (en que cola_bloqueados esta)
static char* buscar_recurso_esperado(t_pcb* pcb, t_dictionary* dict_recursos)
{
    t_list* keys = dictionary_keys(dict_recursos);
    char* resultado = NULL;

    for (int i = 0; i < list_size(keys) && !resultado; i++) {
        char* nombre = list_get(keys, i);
        t_recurso* rec = dictionary_get(dict_recursos, nombre);

        pthread_mutex_lock(&rec->mutex);
        for (int j = 0; j < list_size(rec->cola_bloqueados); j++) {
            t_pcb* bloqueado = list_get(rec->cola_bloqueados, j);
            if (bloqueado->pid == pcb->pid) {
                resultado = nombre;
                break;
            }
        }
        pthread_mutex_unlock(&rec->mutex);
    }

    list_destroy(keys);
    return resultado;
}

// Buscar que procesos tienen una instancia de un recurso dado
static t_list* buscar_poseedores(const char* nombre_recurso, t_list* todos_los_procesos)
{
    t_list* poseedores = list_create();
    for (int i = 0; i < list_size(todos_los_procesos); i++) {
        t_pcb* pcb = list_get(todos_los_procesos, i);
        if (!pcb->recursos_adquiridos) continue;
        for (int j = 0; j < list_size(pcb->recursos_adquiridos); j++) {
            char* res = list_get(pcb->recursos_adquiridos, j);
            if (strcmp(res, nombre_recurso) == 0) {
                uint32_t* pid_ptr = malloc(sizeof(uint32_t));
                *pid_ptr = pcb->pid;
                list_add(poseedores, pid_ptr);
                break;
            }
        }
    }
    return poseedores;
}

// DFS para detectar ciclos
// Retorna true si se encontro un ciclo
static bool dfs_ciclo(t_nodo_grafo** nodos, int num_nodos, int actual,
                       int* estado, int* padre, int* pila_ciclo, int* tam_ciclo)
{
    // estado: 0=no visitado, 1=en stack, 2=completado
    estado[actual] = 1;

    t_nodo_grafo* nodo = nodos[actual];
    for (int i = 0; i < list_size(nodo->espera_a); i++) {
        uint32_t* pid_dest = list_get(nodo->espera_a, i);

        // Buscar indice del nodo destino
        int idx_dest = -1;
        for (int j = 0; j < num_nodos; j++) {
            if (nodos[j]->pid == *pid_dest) {
                idx_dest = j;
                break;
            }
        }
        if (idx_dest < 0) continue;

        if (estado[idx_dest] == 1) {
            // Ciclo encontrado! Reconstruir el ciclo
            *tam_ciclo = 0;
            pila_ciclo[(*tam_ciclo)++] = idx_dest;
            int k = actual;
            while (k != idx_dest && k >= 0 && *tam_ciclo < num_nodos) {
                pila_ciclo[(*tam_ciclo)++] = k;
                k = padre[k];
            }
            return true;
        }

        if (estado[idx_dest] == 0) {
            padre[idx_dest] = actual;
            if (dfs_ciclo(nodos, num_nodos, idx_dest, estado, padre, pila_ciclo, tam_ciclo))
                return true;
        }
    }

    estado[actual] = 2;
    return false;
}

bool grafo_espera_detectar_deadlock(void)
{
    t_dictionary* dict_recursos = recursos_obtener_diccionario();
    if (!dict_recursos) return false;

    log_inicio_deadlock();

    // Recopilar todos los procesos activos
    t_list* todos = list_create();

    pthread_mutex_lock(&mutex_ready);
    for (int i = 0; i < list_size(cola_ready); i++)
        list_add(todos, list_get(cola_ready, i));
    pthread_mutex_unlock(&mutex_ready);

    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++)
        list_add(todos, list_get(cola_exec, i));
    pthread_mutex_unlock(&mutex_exec);

    pthread_mutex_lock(&mutex_blocked);
    for (int i = 0; i < list_size(cola_blocked); i++)
        list_add(todos, list_get(cola_blocked, i));
    pthread_mutex_unlock(&mutex_blocked);

    int num_procesos = list_size(todos);
    if (num_procesos == 0) {
        list_destroy(todos);
        return false;
    }

    // Construir nodos del grafo
    t_nodo_grafo** nodos = calloc(num_procesos, sizeof(t_nodo_grafo*));
    int num_nodos_bloqueados = 0;

    for (int i = 0; i < num_procesos; i++) {
        t_pcb* pcb = list_get(todos, i);
        t_nodo_grafo* nodo = malloc(sizeof(t_nodo_grafo));
        nodo->pid = pcb->pid;
        nodo->pcb = pcb;
        nodo->recurso_esperado = NULL;
        nodo->espera_a = list_create();

        if (pcb->estado == BLOCK) {
            char* recurso = buscar_recurso_esperado(pcb, dict_recursos);
            if (recurso) {
                nodo->recurso_esperado = recurso;
                t_list* poseedores = buscar_poseedores(recurso, todos);
                // Copiar a espera_a
                for (int j = 0; j < list_size(poseedores); j++) {
                    uint32_t* pid_ptr = list_get(poseedores, j);
                    if (*pid_ptr != pcb->pid) { // No apuntar a si mismo
                        list_add(nodo->espera_a, pid_ptr);
                    } else {
                        free(pid_ptr);
                    }
                }
                list_destroy(poseedores);
                num_nodos_bloqueados++;
            }
        }

        nodos[i] = nodo;
    }

    // Si no hay nodos bloqueados, no puede haber deadlock
    bool deadlock = false;
    if (num_nodos_bloqueados < 2) goto cleanup;

    // DFS para detectar ciclos
    {
        int* estado = calloc(num_procesos, sizeof(int));
        int* padre = calloc(num_procesos, sizeof(int));
        int* pila_ciclo = calloc(num_procesos, sizeof(int));
        int tam_ciclo = 0;

        for (int i = 0; i < num_procesos; i++) padre[i] = -1;

        for (int i = 0; i < num_procesos; i++) {
            if (estado[i] == 0 && list_size(nodos[i]->espera_a) > 0) {
                if (dfs_ciclo(nodos, num_procesos, i, estado, padre, pila_ciclo, &tam_ciclo)) {
                    deadlock = true;

                    // Loguear cada proceso en el ciclo
                    for (int c = 0; c < tam_ciclo; c++) {
                        int idx = pila_ciclo[c];
                        t_nodo_grafo* n = nodos[idx];
                        log_deadlock_detectado(
                            n->pid,
                            n->pcb->recursos_adquiridos,
                            n->recurso_esperado ? n->recurso_esperado : "?");
                    }
                    break;
                }
            }
        }

        free(estado);
        free(padre);
        free(pila_ciclo);
    }

cleanup:
    for (int i = 0; i < num_procesos; i++) {
        t_nodo_grafo* nodo = nodos[i];
        list_destroy_and_destroy_elements(nodo->espera_a, free);
        free(nodo);
    }
    free(nodos);
    list_destroy(todos);

    if (deadlock) {
        log_info(KERNEL_CTX.logger, "Deadlock detectado en el sistema");
    } else {
        log_info(KERNEL_CTX.logger, "No se detecto deadlock");
    }

    return deadlock;
}
