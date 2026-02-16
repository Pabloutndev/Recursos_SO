#include <deteccion_deadlock/banquero.h>
#include <peticiones/recursos.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <mod_kernel.h>

#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>
#include <stdlib.h>

bool banquero_estado_seguro(void)
{
    t_dictionary* dict_recursos = recursos_obtener_diccionario();
    if (!dict_recursos) return true;

    // Obtener lista de nombres de recursos
    t_list* nombres = dictionary_keys(dict_recursos);
    int num_recursos = list_size(nombres);
    if (num_recursos == 0) {
        list_destroy(nombres);
        return true;
    }

    // Copiar disponibles
    int* disponible = calloc(num_recursos, sizeof(int));
    for (int r = 0; r < num_recursos; r++) {
        char* nombre = list_get(nombres, r);
        t_recurso* rec = dictionary_get(dict_recursos, nombre);
        disponible[r] = rec->instancias;
    }

    // Recopilar todos los procesos no-EXIT (en READY, EXEC, BLOCKED)
    t_list* procesos = list_create();

    pthread_mutex_lock(&mutex_ready);
    for (int i = 0; i < list_size(cola_ready); i++)
        list_add(procesos, list_get(cola_ready, i));
    pthread_mutex_unlock(&mutex_ready);

    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++)
        list_add(procesos, list_get(cola_exec, i));
    pthread_mutex_unlock(&mutex_exec);

    pthread_mutex_lock(&mutex_blocked);
    for (int i = 0; i < list_size(cola_blocked); i++)
        list_add(procesos, list_get(cola_blocked, i));
    pthread_mutex_unlock(&mutex_blocked);

    int num_procesos = list_size(procesos);
    if (num_procesos == 0) {
        list_destroy(procesos);
        list_destroy(nombres);
        free(disponible);
        return true;
    }

    // Construir matriz de asignacion: asignado[p][r]
    int** asignado = calloc(num_procesos, sizeof(int*));
    for (int p = 0; p < num_procesos; p++) {
        asignado[p] = calloc(num_recursos, sizeof(int));
        t_pcb* pcb = list_get(procesos, p);

        if (pcb->recursos_adquiridos) {
            for (int i = 0; i < list_size(pcb->recursos_adquiridos); i++) {
                char* res_name = list_get(pcb->recursos_adquiridos, i);
                for (int r = 0; r < num_recursos; r++) {
                    if (strcmp(res_name, list_get(nombres, r)) == 0) {
                        asignado[p][r]++;
                        break;
                    }
                }
            }
        }
    }

    // Construir vector de necesidad (request) para procesos bloqueados
    // Un proceso bloqueado en la cola de un recurso necesita 1 instancia de ese recurso.
    int** necesidad = calloc(num_procesos, sizeof(int*));
    for (int p = 0; p < num_procesos; p++) {
        necesidad[p] = calloc(num_recursos, sizeof(int));
        t_pcb* pcb = list_get(procesos, p);

        if (pcb->estado == BLOCK) {
            // Buscar en que cola de recurso esta bloqueado
            for (int r = 0; r < num_recursos; r++) {
                char* nombre = list_get(nombres, r);
                t_recurso* rec = dictionary_get(dict_recursos, nombre);
                pthread_mutex_lock(&rec->mutex);
                for (int j = 0; j < list_size(rec->cola_bloqueados); j++) {
                    t_pcb* bloqueado = list_get(rec->cola_bloqueados, j);
                    if (bloqueado->pid == pcb->pid) {
                        necesidad[p][r] = 1;
                        break;
                    }
                }
                pthread_mutex_unlock(&rec->mutex);
            }
        }
        // Procesos no bloqueados: necesidad = 0 (pueden terminar y liberar)
    }

    // Algoritmo del banquero: simulacion de secuencia segura
    bool* terminado = calloc(num_procesos, sizeof(bool));
    int* work = calloc(num_recursos, sizeof(int));
    memcpy(work, disponible, num_recursos * sizeof(int));

    int completados = 0;
    bool progreso = true;

    while (progreso && completados < num_procesos) {
        progreso = false;
        for (int p = 0; p < num_procesos; p++) {
            if (terminado[p]) continue;

            // Verificar si necesidad[p] <= work
            bool puede = true;
            for (int r = 0; r < num_recursos; r++) {
                if (necesidad[p][r] > work[r]) {
                    puede = false;
                    break;
                }
            }

            if (puede) {
                // Simular terminacion: liberar recursos asignados
                for (int r = 0; r < num_recursos; r++)
                    work[r] += asignado[p][r];
                terminado[p] = true;
                completados++;
                progreso = true;
            }
        }
    }

    bool seguro = (completados == num_procesos);

    // Cleanup
    for (int p = 0; p < num_procesos; p++) {
        free(asignado[p]);
        free(necesidad[p]);
    }
    free(asignado);
    free(necesidad);
    free(terminado);
    free(work);
    free(disponible);
    list_destroy(procesos);
    list_destroy(nombres);

    return seguro;
}
