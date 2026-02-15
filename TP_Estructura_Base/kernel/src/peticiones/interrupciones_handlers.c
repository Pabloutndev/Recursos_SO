#include <peticiones/interrupciones.h>
#include <peticiones/dispatch.h>
#include <peticiones/recursos.h>
#include <planificacion/planificacion.h>
#include <conexiones/memoria.h>
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

void manejar_wait_recurso(t_contexto_cpu* ctx, const char* nombre_recurso) {
    if (!nombre_recurso) {
        log_error(logger, "PID: %d - WAIT error: nombre_recurso NULL", ctx->pid);
        manejar_interrupcion(ctx->pid, MOTIVO_QUANTUM);
        return;
    }

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
        log_error(logger, "PID: %d - WAIT error: PCB no encontrado", ctx->pid);
        return;
    }

    // Actualizar Contexto en PCB
    pcb->program_counter = ctx->pc;
    pcb->registros = ctx->registros;

    // 2. Intentar adquirir recurso
    bool bloqueo = recurso_wait(pcb, (char*)nombre_recurso);

    if (bloqueo) {
        // Bloquear proceso
        manejar_interrupcion(ctx->pid, MOTIVO_WAIT);
    } else {
        // No bloquea - recurso adquirido, volver a Ready
        manejar_interrupcion(ctx->pid, MOTIVO_QUANTUM);
    }
}

void manejar_signal_recurso(t_contexto_cpu* ctx, const char* nombre_recurso) {
    if (!nombre_recurso) {
        log_error(logger, "PID: %d - SIGNAL error: nombre_recurso NULL", ctx->pid);
        manejar_interrupcion(ctx->pid, MOTIVO_QUANTUM);
        return;
    }

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

            log_cambio_estado(desbloqueado->pid, "BLOCKED", "READY");
        } else {
             // Podria no estar en Blocked global si hubo algun race o error.
             log_error(logger, "PID: %d - SIGNAL error: no encontrado en BLOCKED global", desbloqueado->pid);
        }
    }

    // El proceso que hizo SIGNAL (ctx->pid) sigue ejecutando.
    // Actualizar contexto del PCB (PC incrementado por CPU) antes de mover a READY.
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* p = list_get(cola_exec, i);
        if (p->pid == ctx->pid) {
            p->program_counter = ctx->pc;
            p->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    manejar_interrupcion(ctx->pid, MOTIVO_QUANTUM);
}

void manejar_fin_quantum(t_contexto_cpu* ctx)
{
    if (!ctx) return;

    log_info(logger, "PID: %u - Quantum recibido PC=%u", ctx->pid, ctx->pc);

    // PASO 1: Actualizar contexto en el PCB
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* pcb = (t_pcb*) list_get(cola_exec, i);
        if (pcb && pcb->pid == ctx->pid) {
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            log_info(logger, "PID: %u - Contexto actualizado PC=%u", pcb->pid, pcb->program_counter);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    // PASO 2: Notificar a Memoria para actualizar su copia
    // TODO: Implementar solicitud de actualización de contexto en Memoria si es necesario

    // PASO 3: Desalojar por quantum (pasar a READY)
    manejar_interrupcion(ctx->pid, MOTIVO_QUANTUM);
}

void manejar_fin_proceso(t_contexto_cpu* ctx)
{
    if (!ctx) return;

    log_info(logger, "PID: %u - Fin de proceso", ctx->pid);

    // PASO 1: Actualizar contexto en PCB
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

    // PASO 2: Finalizar proceso (mover a EXIT)
    manejar_interrupcion(ctx->pid, MOTIVO_EXIT);

    // PASO 3: Liberar recursos en Memoria
    solicitar_fin_proceso_memoria(ctx->pid);
}
