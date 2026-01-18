#include "contexto_cpu_adapter.h"
#include <model/model.h>
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

    memcpy(cpu_estado, ctx, sizeof(t_contexto_cpu));
    
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
    memcpy(ctx, &cpu_estado, sizeof(t_contexto_cpu));

    // Copiar parámetros
    for (int i = 0; i < 256 && cpu_estado.parametros[i] != '\0'; i++) {
        ctx->parametros[i] = cpu_estado.parametros[i];
    }
    ctx->parametros[255] = '\0';

    log_info(logger, "ADAPTER CONTEXTO: Contexto extraído - PID=%u, PC=%u, MOTIVO=%u",
             ctx->pid, ctx->pc, ctx->motivo_desalojo);

    return ctx;
}
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <ciclo_instruccion/ciclo.h>
#include <interrupciones/interrupciones.h>
#include <instrucciones/instrucciones.h>
#include <string.h>

void cpu_handler_atender_ejecucion(int fd, t_paquete* p)
{
    t_contexto_cpu* ctx = recibir_contexto(p);
    if (!ctx) return;

    log_info(logger, "ADAPTER: Iniciando ejecución PID %u", ctx->pid);

    // ✅ EJECUTAR
    ciclo_instruccion_ejecutar(ctx);

    // ✅ DETERMINAR MOTIVO
    op_code rs_code = OP_DESALOJO;
    if (ctx->finalizado) rs_code = OP_MEM_FIN_PROCESO;
    else if (interrupcion_pendiente()) {
        rs_code = OP_FIN_DE_QUANTUM;
        interrupcion_reset();
    }
    // ... simplificado para demo, pero debería mapear ctx->motivo_desalojo ...

    // ✅ RESPONDER
    enviar_contexto(fd, ctx, rs_code);
    free(ctx);
}

void cpu_handler_atender_interrupcion(int fd, t_paquete* p)
{
    log_info(logger, "ADAPTER: Interrupción recibida");
    interrupcion_disparar(0);
}
