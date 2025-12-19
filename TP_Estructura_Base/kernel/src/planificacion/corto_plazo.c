// corto_plazo.c
#include <planificacion/planificacion.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <planificacion/corto_plazo.h>
#include <peticiones/dispatch.h>
#include <loggers/logger.h>
#include <semaphore.h>
#include <pthread.h>

extern sem_t sem_hay_ready;
extern pthread_mutex_t mutex_exec;
extern t_list* cola_exec;
extern t_list* cola_ready;
extern pthread_mutex_t mutex_ready;
extern t_log* logger;

/* proximoAEjecutar está declarado en planificacion.c como extern */
extern t_pcb* (*proximoAEjecutar)(void);

void* planificador_corto_plazo(void* _) {
    while (1) {
        sem_wait(&sem_hay_ready);

        t_pcb* pcb = proximoAEjecutar();
        if (!pcb) continue;

        pcb->estado = EXEC;
        temporal_stop(pcb->tiempo_ready); // Detener tiempo de espera

        pthread_mutex_lock(&mutex_exec);
        list_add(cola_exec, pcb);
        pthread_mutex_unlock(&mutex_exec);

        log_cambio_estado(pcb->pid, "READY", "EXEC");
        log_info(logger, "READY → EXEC PID=%u", pcb->pid);

        // Despachar proceso a CPU
        int sock = enviar_proceso_a_cpu(pcb);
        if (sock < 0) {
            log_error(logger, "Error al enviar proceso %u a CPU", pcb->pid);
            // Reencolar en READY si falla el envío
            pthread_mutex_lock(&mutex_exec);
            list_remove_element(cola_exec, pcb);
            pthread_mutex_unlock(&mutex_exec);
            
            pcb->estado = READY;
            temporal_resume(pcb->tiempo_ready);
            pthread_mutex_lock(&mutex_ready);
            list_add(cola_ready, pcb);
            pthread_mutex_unlock(&mutex_ready);
            sem_post(&sem_hay_ready);
        }
    }
    return NULL;
}
