// corto_plazo.c
#include "planificacion.h"
#include <commons/log.h>
#include <planificacion/corto_plazo.h>
#include <semaphore.h>
#include <pthread.h>

extern sem_t sem_hay_ready;
extern pthread_mutex_t mutex_exec;
extern t_list* cola_exec;
extern t_log* logger;

/* proximoAEjecutar está declarado en planificacion.c como extern */
extern t_pcb* (*proximoAEjecutar)(void);

/* Nota: dispatch/enviar_a_cpu debe implementarse por separado; aquí solo seleccionamos */
void* planificador_corto_plazo(void* _) {
    while (1) {
        sem_wait(&sem_hay_ready);

        t_pcb* pcb = proximoAEjecutar();
        if (!pcb) continue;

        pcb->estado = EXEC;

        pthread_mutex_lock(&mutex_exec);
        list_add(cola_exec, pcb);
        pthread_mutex_unlock(&mutex_exec);

        log_info(logger, "READY → EXEC PID=%u", pcb->pid);

        /* DESPACHAR:
           Llamá a enviar_a_cpu(pcb) desde otro módulo (dispatch).
           Este hilo NO debe bloquear esperando la respuesta de CPU aquí,
           la recepción y reencolado se manejará por el módulo de recepción.
        */
    }
    return NULL;
}
