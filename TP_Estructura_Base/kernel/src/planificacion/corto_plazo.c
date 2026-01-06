// corto_plazo.c
#include <loggers/logger.h>
#include <planificacion/planificacion.h>
#include <planificacion/corto_plazo.h>
#include <peticiones/dispatch.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>

#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

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

        if (sock >= 0) {
            // Lanzar timer de quantum
            pthread_t hilo_quantum;
            pthread_create(&hilo_quantum, NULL, timer_quantum, pcb);
            pthread_detach(hilo_quantum);
        } else if (sock < 0) {
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

void* timer_quantum(void* arg) {
    t_pcb* pcb = (t_pcb*) arg;

    // Usamos el quantum propio del PCB en microsegundos
    usleep(pcb->quantum);

    bool sigue_en_exec = false;

    // Verificamos si sigue en EXEC usando pid
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* aux = list_get(cola_exec, i);
        if (aux->pid == pcb->pid) { // identidad por PID
            sigue_en_exec = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    if (sigue_en_exec) {
        // TODO: 
        //enviar_interrupcion(pcb->pid);
        log_info(logger, "Quantum vencido → PID=%u desalojado", pcb->pid);
    }

    return NULL;
}

