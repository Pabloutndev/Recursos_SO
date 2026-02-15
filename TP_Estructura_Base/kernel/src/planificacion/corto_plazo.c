// corto_plazo.c
#include <loggers/logger.h>
#include <planificacion/planificacion.h>
#include <planificacion/corto_plazo.h>
#include <peticiones/dispatch.h>
#include <config/kernel_config.h>
#include <mod_kernel.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
        planificacion_check_pause();

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

        // ✅ DESPACHAR A CPU para ejecución
        int sock = enviar_proceso_a_cpu(pcb);

        if (sock >= 0) {
            // ✅ LANZAR TIMER solo si el algoritmo es RR o VRR (FIFO no usa quantum)
            bool es_rr = (strcmp(KERNEL_CTX.config.algoritmo_planificacion, "RR") == 0);
            bool es_vrr = (strcmp(KERNEL_CTX.config.algoritmo_planificacion, "VRR") == 0);
            bool usa_quantum = es_rr || es_vrr;

            if (usa_quantum) {
                // VRR usa quantum_restante, RR siempre usa quantum completo
                int quantum_a_usar = es_vrr ? pcb->quantum_restante : pcb->quantum;
                if (quantum_a_usar <= 0) quantum_a_usar = pcb->quantum; // fallback

                typedef struct { uint32_t pid; int quantum; } t_quantum_args;
                t_quantum_args* q_args = malloc(sizeof(t_quantum_args));
                q_args->pid = pcb->pid;
                q_args->quantum = quantum_a_usar;

                // Guardar timestamp de inicio de ejecucion para calcular quantum consumido
                if (pcb->tiempo_inicio_exec) temporal_destroy(pcb->tiempo_inicio_exec);
                pcb->tiempo_inicio_exec = temporal_create();

                pthread_t hilo_quantum;
                pthread_create(&hilo_quantum, NULL, timer_quantum, q_args);
                pthread_detach(hilo_quantum);

                log_info(logger, "Planificador: Timer quantum iniciado para PID=%u (%d ms, %s)",
                         pcb->pid, quantum_a_usar, es_vrr ? "VRR" : "RR");
            } else {
                log_info(logger, "Planificador: Algoritmo sin quantum (FIFO/HRRN) - PID=%u ejecuta sin desalojo temporal", pcb->pid);
            }

            // ✅ ESPERAR RESPUESTA DE CPU (Bloqueante)
            atender_dispatch_cpu();
            
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
    struct { uint32_t pid; int quantum; }* q_args = arg;
    uint32_t pid = q_args->pid;
    int quantum_ms = q_args->quantum;
    free(q_args);

    // Usamos el quantum propio del PCB convertido a microsegundos
    usleep(quantum_ms * 1000);

    // Verificar que el algoritmo actual usa quantum (podria haber cambiado a FIFO)
    bool usa_quantum = (strcmp(KERNEL_CTX.config.algoritmo_planificacion, "RR") == 0) ||
                       (strcmp(KERNEL_CTX.config.algoritmo_planificacion, "VRR") == 0);
    if (!usa_quantum) {
        log_info(logger, "Timer quantum PID=%u: algoritmo ya no usa quantum, ignorando", pid);
        return NULL;
    }

    bool sigue_en_exec = false;

    // Verificamos si sigue en EXEC usando pid
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* aux = list_get(cola_exec, i);
        if (aux->pid == pid) {
            sigue_en_exec = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    if (sigue_en_exec) {
        log_info(logger, "Quantum vencido → PID=%u desalojado", pid);
        enviar_interrupt_cpu(pid);
    }

    return NULL;
}

