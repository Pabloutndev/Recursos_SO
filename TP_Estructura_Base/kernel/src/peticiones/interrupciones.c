#include <peticiones/interrupciones.h>
#include <peticiones/dispatch.h>
#include <peticiones/recursos.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/logger.h>
#include <mod_kernel.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <string.h>

#define MOTIVO_QUANTUM "QUANTUM"
#define MOTIVO_IO "IO"
#define MOTIVO_EXIT "EXIT"
#define MOTIVO_WAIT "WAIT"

// Declaraciones de funciones externas
extern void log_fin_quantum(int pid);
extern void log_bloqueo(int pid, const char* motivo);
extern void log_fin_proceso(int pid, const char* motivo);

void desalojar_proceso(uint32_t pid)
{
    pthread_mutex_lock(&mutex_exec);

    t_pcb* pcb = NULL;
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* p = list_get(cola_exec, i);
        if (p->pid == pid) {
            pcb = p;
            break;
        }
    }

    if (pcb) {
        // Enviar interrupción a CPU
        enviar_interrupt_cpu(pid);
        log_fin_quantum(pid);

        // Cambiar estado a READY
        pcb->estado = READY;
        list_remove_element(cola_exec, pcb);

        pthread_mutex_unlock(&mutex_exec);

        // Reencolar en READY
        pthread_mutex_lock(&mutex_ready);
        list_add(cola_ready, pcb);
        temporal_resume(pcb->tiempo_ready);
        pthread_mutex_unlock(&mutex_ready);

        sem_post(&sem_hay_ready);
    } else {
        pthread_mutex_unlock(&mutex_exec);
        log_error(logger, "PID: %u - No esta en ejecucion", pid);
    }
}

void manejar_interrupcion(uint32_t pid, const char* motivo)
{
    log_info(logger, "PID: %u - Interrupcion: %s", pid, motivo);

    pthread_mutex_lock(&mutex_exec);

    t_pcb* pcb = NULL;
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* p = list_get(cola_exec, i);
        if (p->pid == pid) {
            pcb = p;
            break;
        }
    }

    if (!pcb) {
        pthread_mutex_unlock(&mutex_exec);
        return;
    }

    list_remove_element(cola_exec, pcb);
    pthread_mutex_unlock(&mutex_exec);

    if (strcmp(motivo, MOTIVO_QUANTUM) == 0) {
        // Desalojo por quantum - VRR: resetear quantum_restante al completo
        pcb->quantum_restante = pcb->quantum;
        pcb->estado = READY;
        pcb->tiempo_ready = temporal_create();
        pthread_mutex_lock(&mutex_ready);
        list_add(cola_ready, pcb);
        pthread_mutex_unlock(&mutex_ready);
        sem_post(&sem_hay_ready);
        log_cambio_estado(pid, "EXEC", "READY");
    } else if (strcmp(motivo, MOTIVO_IO) == 0 || strcmp(motivo, MOTIVO_WAIT) == 0) {
        // Bloqueo por I/O o wait - VRR: calcular quantum consumido y guardar restante
        if (pcb->tiempo_inicio_exec) {
            int64_t tiempo_exec_ms = temporal_gettime(pcb->tiempo_inicio_exec);
            int restante = pcb->quantum_restante - (int)tiempo_exec_ms;
            pcb->quantum_restante = (restante > 0) ? restante : 0;
            temporal_destroy(pcb->tiempo_inicio_exec);
            pcb->tiempo_inicio_exec = NULL;
            log_info(logger, "PID: %u - VRR bloqueado, quantum_restante=%d ms", pid, pcb->quantum_restante);
        }
        pcb->estado = BLOCK;
        pthread_mutex_lock(&mutex_blocked);
        list_add(cola_blocked, pcb);
        pthread_mutex_unlock(&mutex_blocked);
        log_bloqueo(pid, motivo);
    } else if (strcmp(motivo, MOTIVO_EXIT) == 0) {
        // Proceso terminó
        pcb->estado = EXIT;
        pthread_mutex_lock(&mutex_exit);
        list_add(cola_exit, pcb);
        pthread_mutex_unlock(&mutex_exit);
        log_fin_proceso(pid, "SUCCESS");
        sem_post(&sem_mp); // Liberar slot de multiprogramacion
    }
}

void manejar_bloqueo_io(t_contexto_cpu* ctx) {
    // Logica basica: Mover a Block
    // TODO: Usar interfaz IO
    manejar_interrupcion(ctx->pid, MOTIVO_IO);
}
