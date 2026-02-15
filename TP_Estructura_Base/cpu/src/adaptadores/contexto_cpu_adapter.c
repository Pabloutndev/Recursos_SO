#include <adaptadores/contexto_cpu_adapter.h>
#include <model/model.h>
#include <cpu.h>    // Acceso a cpu_estado global
#include <stdlib.h>
#include <commons/log.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <paquete/paquete.h>
#include <serializacion/serializacion.h>
#include <conexion/conexion.h>
#include <string.h>

extern t_contexto_cpu cpu_estado;
extern t_log* logger;
extern int socket_dispatch;

bool recibir_contexto_kernel(t_contexto_cpu* ctx)
{
    t_paquete* paquete = recibir_paquete(socket_dispatch);
    if (!paquete) {
        log_error(logger, "Fallo al recibir paquete de Kernel");
        return false;
    }

    if (paquete->codigo_operacion != OP_PROCESO_EXEC) {
        log_warning(logger,
            "OpCode inesperado (%d) al recibir contexto",
            paquete->codigo_operacion
        );
        paquete_destroy(paquete);
        return false;
    }

    t_contexto_cpu* recibido = recibir_contexto(paquete);
    paquete_destroy(paquete);

    if (!recibido) {
        log_error(logger, "Contexto recibido NULL");
        return false;
    }

    // Copia profunda (estructura plana)
    *ctx = *recibido;
    free(recibido);

    log_info(logger, "PID: %d - Contexto recibido PC=%d", ctx->pid, ctx->pc);
    return true;
}

void enviar_contexto_kernel(t_contexto_cpu* ctx, t_motivo_desalojo motivo)
{
    op_code rs_code;
    switch (motivo) {
        case MOTIVO_EXIT:     rs_code = OP_CPU_FIN_PROCESO; break;
        case MOTIVO_IO:       rs_code = OP_BLOQUEO_IO; break;
        case MOTIVO_SEGFAULT: rs_code = OP_SEGFAULT; break;
        default:              rs_code = OP_DESALOJO; break;
    }

    enviar_contexto(socket_dispatch, ctx, rs_code);

    log_info(logger,
        "PID: %d - Contexto enviado PC=%d motivo=%d",
        ctx->pid,
        ctx->pc,
        motivo
    );
}

/* ========================================
 * OPERACIONES
 * ======================================== */

void cpu_contexto_adapter_cargar(t_contexto_cpu* ctx)
{
    if (!ctx) {
        log_error(logger, "Contexto NULL al cargar");
        return;
    }

    memcpy(&cpu_estado, ctx, sizeof(t_contexto_cpu));

    log_info(logger, "PID: %u - Contexto cargado PC=%u",
             ctx->pid, ctx->pc);
}

t_contexto_cpu* cpu_contexto_adapter_extraer(void)
{
    // Crear nuevo contexto con estado actual de CPU
    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));
    memcpy(ctx, &cpu_estado, sizeof(t_contexto_cpu));

    log_info(logger, "PID: %u - Contexto extraido PC=%u",
             ctx->pid, ctx->pc);

    return ctx;
}
