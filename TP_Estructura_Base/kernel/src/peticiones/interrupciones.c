#include <peticiones/interrupciones.h>
#include <peticiones/dispatch.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/logger.h>
#include <mod_kernel.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <string.h>

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
        log_error(logger, "Proceso %u no está en ejecución", pid);
    }
}

void manejar_interrupcion(uint32_t pid, const char* motivo)
{
    log_info(logger, "Interrupción recibida - PID: %u, Motivo: %s", pid, motivo);
    
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
    
    if (strcmp(motivo, "QUANTUM") == 0) {
        // Desalojo por quantum
        pcb->estado = READY;
        pthread_mutex_lock(&mutex_ready);
        list_add(cola_ready, pcb);
        temporal_resume(pcb->tiempo_ready);
        pthread_mutex_unlock(&mutex_ready);
        sem_post(&sem_hay_ready);
    } else if (strcmp(motivo, "IO") == 0 || strcmp(motivo, "WAIT") == 0) {
        // Bloqueo por I/O o wait
        pcb->estado = BLOCK;
        pthread_mutex_lock(&mutex_blocked);
        list_add(cola_blocked, pcb);
        pthread_mutex_unlock(&mutex_blocked);
        log_bloqueo(pid, motivo);
    } else if (strcmp(motivo, "EXIT") == 0) {
        // Proceso terminó
        pcb->estado = EXIT;
        pthread_mutex_lock(&mutex_exec);
        list_add(cola_exit, pcb);
        pthread_mutex_unlock(&mutex_exec);
        log_fin_proceso(pid, "EXIT");
    }
}