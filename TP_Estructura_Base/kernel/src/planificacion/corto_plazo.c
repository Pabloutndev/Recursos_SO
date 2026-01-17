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

/*
 * Planificador a Corto Plazo (CPU Scheduler)
 * 
 * Responsabilidades:
 * 1. Esperar que haya procesos en READY (sem_wait)
 * 2. Seleccionar próximo proceso según algoritmo
 * 3. Cambiar estado READY → EXEC
 * 4. Enviar a CPU para ejecución
 * 5. Lanzar timer de quantum (en hilo separado)
 * 6. Esperar respuesta de CPU (en hilo listener)
 * 
 * Flujo:
 * READY → EXEC (aquí) → respuesta CPU → READY/BLOCK/EXIT (listener)
 */
void* planificador_corto_plazo(void* _) {
    while (1) {
        // ✅ ESPERAR procesos disponibles
        sem_wait(&sem_hay_ready);

        // ✅ SELECCIONAR según algoritmo (FIFO/RR/HRRN)
        t_pcb* pcb = proximoAEjecutar();
        if (!pcb) {
            log_warning(logger, "Planificador: Proceso NULL retornado por proximoAEjecutar");
            continue;
        }

        // ✅ CAMBIAR ESTADO Y TIMESTAMP
        pcb->estado = EXEC;
        temporal_stop(pcb->tiempo_ready); // Detener cronómetro de READY

        // ✅ AGREGAR A EXEC bajo lock
        pthread_mutex_lock(&mutex_exec);
        list_add(cola_exec, pcb);
        pthread_mutex_unlock(&mutex_exec);

        log_cambio_estado(pcb->pid, "READY", "EXEC");
        log_info(logger, "Planificador: READY → EXEC PID=%u, Quantum=%u", pcb->pid, pcb->quantum);

        // ✅ DESPACHAR A CPU para ejecución
        int sock = enviar_proceso_a_cpu(pcb);

        if (sock >= 0) {
            // ✅ LANZAR TIMER en hilo separado (no bloquea)
            pthread_t hilo_quantum;
            pthread_create(&hilo_quantum, NULL, timer_quantum, (void*)(uintptr_t)pcb->pid);
            pthread_detach(hilo_quantum);
            
            log_info(logger, "Planificador: Timer quantum iniciado para PID=%u", pcb->pid);
            
        } else if (sock < 0) {
            log_error(logger, "Planificador: Fallo envío a CPU para PID=%u", pcb->pid);
            
            // ✅ RECUPERACIÓN: Reencolar en READY
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
        enviar_interrupt_cpu(pcb->pid);
        log_info(logger, "Quantum vencido → PID=%u desalojado", pcb->pid);
    }

    return NULL;
}

