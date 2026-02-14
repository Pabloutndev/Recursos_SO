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
        // Desalojo por quantum - VRR: resetear quantum_restante al completo
        pcb->quantum_restante = pcb->quantum;
        pcb->estado = READY;
        pcb->tiempo_ready = temporal_create();
        pthread_mutex_lock(&mutex_ready);
        list_add(cola_ready, pcb);
        pthread_mutex_unlock(&mutex_ready);
        sem_post(&sem_hay_ready);
        log_info(logger, "PID: %u - Movido EXEC -> READY (Fin Quantum)", pid);
    } else if (strcmp(motivo, "IO") == 0 || strcmp(motivo, "WAIT") == 0) {
        // Bloqueo por I/O o wait - VRR: calcular quantum consumido y guardar restante
        if (pcb->tiempo_ready) {
            int64_t tiempo_exec_ms = temporal_gettime(pcb->tiempo_ready);
            int restante = pcb->quantum_restante - (int)tiempo_exec_ms;
            pcb->quantum_restante = (restante > 0) ? restante : 0;
            temporal_destroy(pcb->tiempo_ready);
            pcb->tiempo_ready = NULL;
            log_info(logger, "VRR: PID=%u bloqueado, quantum_restante=%d ms", pid, pcb->quantum_restante);
        }
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
        log_info(logger, "PID: %u - Finalizado (EXIT) - Movido a cola EXIT", pid);
        log_fin_proceso(pid, "SUCCESS");
        sem_post(&sem_mp); // Liberar slot de multiprogramacion
    }
}

void manejar_bloqueo_io(t_contexto_cpu* ctx) {
    // Logica basica: Mover a Block
    // TODO: Usar interfaz IO
    manejar_interrupcion(ctx->pid, "IO");
}

void manejar_wait_recurso(t_contexto_cpu* ctx, const char* nombre_recurso) {
    // 1. Buscar PCB en EXEC
    t_pcb* pcb = NULL;
    pthread_mutex_lock(&mutex_exec);
    for(int i=0; i<list_size(cola_exec); i++) {
        t_pcb* p = list_get(cola_exec, i);
        if(p->pid == ctx->pid) {
            pcb = p;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    if (!pcb) {
        log_error(logger, "WAIT: PCB no encontrado para PID %d", ctx->pid);
        return;
    }

    // Actualizar Contexto en PCB
    pcb->program_counter = ctx->pc;
    pcb->registros = ctx->registros;

    // 2. Intentar adquirir recurso
    bool bloqueo = recurso_wait(pcb, (char*)nombre_recurso);

    if (bloqueo) {
        // Bloquear proceso
        manejar_interrupcion(ctx->pid, "WAIT");
    } else {
        // No bloquea - recurso adquirido, volver a Ready
        manejar_interrupcion(ctx->pid, "QUANTUM");
    }
}

void manejar_signal_recurso(t_contexto_cpu* ctx, const char* nombre_recurso) {
    t_pcb* desbloqueado = recurso_signal((char*)nombre_recurso);
    
    if (desbloqueado) {
        // El proceso desbloqueado estaba en BLOCKED y en la cola del recurso.
        // recurso_signal lo sacó de la cola del recurso.
        // Ahora debemos sacarlo de BLOCKED global y pasarlo a READY.
        
        pthread_mutex_lock(&mutex_blocked);
        // OJO: list_remove_element usa comparacion de punteros. 
        // Si desbloqueado es el puntero real, funciona.
        bool removed = list_remove_element(cola_blocked, desbloqueado);
        pthread_mutex_unlock(&mutex_blocked);
        
        if (removed) {
            desbloqueado->estado = READY;
            desbloqueado->tiempo_ready = temporal_create(); // Reset wait time?
            
            pthread_mutex_lock(&mutex_ready);
            list_add(cola_ready, desbloqueado);
            pthread_mutex_unlock(&mutex_ready);
            sem_post(&sem_hay_ready);
            
            log_info(logger, "PID %d Movido de BLOCKED a READY por SIGNAL", desbloqueado->pid);
        } else {
             // Podria no estar en Blocked global si hubo algun race o error.
             log_error(logger, "PID %d desbloqueado por recurso pero no encontrado en BLOCKED global", desbloqueado->pid);
        }
    }
    
    // El proceso que hizo SIGNAL (ctx->pid) sigue ejecutando. 
    // Como CPU devolvió control, lo mandamos a Ready para que siga compitiendo (o Dispatch directo).
    // Simil Wait exitoso.
    manejar_interrupcion(ctx->pid, "QUANTUM");
}

void manejar_fin_quantum(t_contexto_cpu* ctx)
{
    if (!ctx) return;
    
    log_info(logger, "CPU: Fin de Quantum para PID=%u, PC=%u", ctx->pid, ctx->pc);
    
    // ✅ PASO 1: Actualizar contexto en el PCB
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* pcb = (t_pcb*) list_get(cola_exec, i);
        if (pcb && pcb->pid == ctx->pid) {
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            log_info(logger, "Contexto actualizado para PID=%u (PC=%u)", pcb->pid, pcb->program_counter);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);
    
    // ✅ PASO 2: Notificar a Memoria para actualizar su copia
    // TODO: Implementar solicitud de actualización de contexto en Memoria si es necesario
    
    // ✅ PASO 3: Desalojar por quantum (pasar a READY)
    manejar_interrupcion(ctx->pid, "QUANTUM");
}

void manejar_fin_proceso(t_contexto_cpu* ctx)
{
    if (!ctx) return;
    
    log_info(logger, "CPU: Fin de Proceso PID=%u", ctx->pid);
    
    // ✅ PASO 1: Actualizar contexto en PCB
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* pcb = (t_pcb*) list_get(cola_exec, i);
        if (pcb && pcb->pid == ctx->pid) {
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);
    
    // ✅ PASO 2: Finalizar proceso (mover a EXIT)
    manejar_interrupcion(ctx->pid, "EXIT");
    
    // ✅ PASO 3: Liberar recursos en Memoria
    solicitar_fin_proceso_memoria(ctx->pid);
}