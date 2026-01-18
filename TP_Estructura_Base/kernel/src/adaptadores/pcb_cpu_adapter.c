#include "pcb_cpu_adapter.h"
#include <model/model.h>
#include <pcb/pcb.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <stdlib.h>
#include <commons/log.h>
#include <mod_kernel.h>

extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern t_log* logger;

/* ========================================
 * TRANSFORMACIÓN DE ESTRUCTURAS
 * ======================================== */

t_contexto_cpu* pcb_a_contexto_cpu(t_pcb* pcb)
{
    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));

    ctx->pid = pcb->pid;
    ctx->pc = pcb->program_counter;
    ctx->quantum = pcb->quantum;
    ctx->registros = pcb->registros;
    
    // Inicializar campos de estado
    ctx->finalizado = 0;
    ctx->bloqueado = 0;
    ctx->io_time = 0;
    ctx->motivo_desalojo = 0;
    ctx->parametros[0] = '\0';

    return ctx;
}

void contexto_cpu_a_pcb(t_contexto_cpu* ctx, t_pcb* pcb)
{
    pcb->program_counter = ctx->pc;
    pcb->registros = ctx->registros;
    // Nota: NO modificamos PID, quantum (responsabilidad de Kernel)
}

/* ========================================
 * OPERACIONES COMPLETAS
 * ======================================== */

void kernel_dispatch(t_pcb* pcb)
{
    if (!pcb) {
        log_error(logger, "ADAPTER: PCB nulo en dispatch");
        return;
    }

    // Paso 1: Convertir PCB a Contexto CPU
    t_contexto_cpu* ctx = pcb_a_contexto_cpu(pcb);

    // Paso 2: Enviar a CPU usando protocolo
    log_info(logger, "ADAPTER: Enviando OP_PROCESO_EXEC a CPU (PID=%u, PC=%u)",
             ctx->pid, ctx->pc);
    enviar_contexto(socket_cpu_dispatch, ctx, OP_PROCESO_EXEC);

    // Nota: La respuesta se maneja en el hilo de escucha de CPU Dispatch
    // (ver conexiones/cpu.c - escuchar_dispatch())
    
    free(ctx);
}

void kernel_interrupt(void)
{
    // Enviar interrupción simple (sin payload)
    log_info(logger, "ADAPTER: Enviando OP_INTERRUPCION_CPU");
    enviar_interrupcion_cpu(socket_cpu_interrupt);
    // CPU Interrupt es un socket separado, recibe la "señal" y desaloja
}

