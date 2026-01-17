#include "contexto_cpu_adapter.h"
#include <common/cpu/contexto.h>
#include <cpu.h>    // Acceso a cpu_estado global
#include <stdlib.h>
#include <commons/log.h>

extern t_contexto_cpu cpu_estado;
extern t_log* logger;

/* ========================================
 * OPERACIONES
 * ======================================== */

void cpu_contexto_adapter_cargar(t_contexto_cpu* ctx)
{
    if (!ctx) {
        log_error(logger, "ADAPTER CONTEXTO: Contexto NULL");
        return;
    }

    // Copiar contexto recibido de Kernel al estado de CPU
    cpu_estado.pid = ctx->pid;
    cpu_estado.pc = ctx->pc;
    cpu_estado.quantum = ctx->quantum;
    cpu_estado.registros = ctx->registros;
    
    // Campos de estado (se actualizarán durante ejecución)
    cpu_estado.finalizado = ctx->finalizado;
    cpu_estado.bloqueado = ctx->bloqueado;
    cpu_estado.io_time = ctx->io_time;
    cpu_estado.motivo_desalojo = ctx->motivo_desalojo;
    
    if (ctx->parametros[0] != '\0') {
        // Copiar parámetros si existen (recurso, archivo, etc.)
        for (int i = 0; i < 256 && ctx->parametros[i] != '\0'; i++) {
            cpu_estado.parametros[i] = ctx->parametros[i];
        }
    }

    log_info(logger, "ADAPTER CONTEXTO: Contexto cargado - PID=%u, PC=%u, Q=%u",
             ctx->pid, ctx->pc, ctx->quantum);
}

t_contexto_cpu* cpu_contexto_adapter_extraer(void)
{
    // Crear nuevo contexto con estado actual de CPU
    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));
    
    ctx->pid = cpu_estado.pid;
    ctx->pc = cpu_estado.pc;
    ctx->quantum = cpu_estado.quantum;
    ctx->registros = cpu_estado.registros;
    ctx->finalizado = cpu_estado.finalizado;
    ctx->bloqueado = cpu_estado.bloqueado;
    ctx->io_time = cpu_estado.io_time;
    ctx->motivo_desalojo = cpu_estado.motivo_desalojo;

    // Copiar parámetros
    for (int i = 0; i < 256 && cpu_estado.parametros[i] != '\0'; i++) {
        ctx->parametros[i] = cpu_estado.parametros[i];
    }
    ctx->parametros[255] = '\0';

    log_info(logger, "ADAPTER CONTEXTO: Contexto extraído - PID=%u, PC=%u, MOTIVO=%u",
             ctx->pid, ctx->pc, ctx->motivo_desalojo);

    return ctx;
}
