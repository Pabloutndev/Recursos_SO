#include <adaptadores/contexto_cpu_adapter.h>
#include <model/model.h>
#include <cpu.h>    // Acceso a cpu_estado global
#include <stdlib.h>
#include <commons/log.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <ciclo_instruccion/ciclo.h>
#include <interrupciones/interrupciones.h>
#include <instrucciones/instrucciones.h>
#include <string.h>

extern t_contexto_cpu cpu_estado;
extern t_log* logger;
extern int socket_dispatch;

bool recibir_contexto_kernel(t_contexto_cpu* ctx)
{
    t_paquete* paquete = recibir_paquete(socket_dispatch);
    if (!paquete) {
        log_error(logger, "CPU: Fallo al recibir paquete de Kernel");
        return false;
    }

    if (paquete->codigo_operacion != OP_PROCESO_EXEC) {
        log_warning(logger,
            "CPU: OpCode inesperado (%d) al recibir contexto",
            paquete->codigo_operacion
        );
        paquete_destroy(paquete);
        return false;
    }

    t_contexto_cpu* recibido = recibir_contexto(paquete);
    paquete_destroy(paquete);

    if (!recibido) {
        log_error(logger, "CPU: Contexto recibido NULL");
        return false;
    }

    // Copia profunda (estructura plana)
    *ctx = *recibido;
    free(recibido);

    log_info(logger, "CPU: Contexto recibido PID=%d PC=%d", ctx->pid, ctx->pc);
    return true;
}

void enviar_contexto_kernel(t_contexto_cpu* ctx, t_motivo_desalojo motivo)
{
    enviar_contexto(socket_dispatch, ctx, OP_PROCESO_EXEC);

    log_info(logger,
        "CPU: Contexto enviado PID=%d PC=%d MOTIVO=%d",
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
        log_error(logger, "ADAPTER CONTEXTO: Contexto NULL");
        return;
    }

    memcpy(&cpu_estado, ctx, sizeof(t_contexto_cpu));
    
    log_info(logger, "ADAPTER CONTEXTO: Contexto cargado - PID=%u, PC=%u",
             ctx->pid, ctx->pc);
}

t_contexto_cpu* cpu_contexto_adapter_extraer(void)
{
    // Crear nuevo contexto con estado actual de CPU
    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));
    memcpy(ctx, &cpu_estado, sizeof(t_contexto_cpu));

    log_info(logger, "ADAPTER CONTEXTO: Contexto extraído - PID=%u, PC=%u",
             ctx->pid, ctx->pc);

    return ctx;
}

void cpu_handler_atender_ejecucion(int fd, t_paquete* p)
{
    t_contexto_cpu* ctx = recibir_contexto(p);
    if (!ctx) return;

    log_info(logger, "ADAPTER: Iniciando ejecución PID %u", ctx->pid);

    // ✅ EJECUTAR
    ciclo_instruccion_ejecutar(ctx);

    // ✅ DETERMINAR MOTIVO
    op_code rs_code = OP_DESALOJO;
    if (interrupcion_pendiente()) {
        rs_code = OP_FIN_DE_QUANTUM;
        interrupcion_reset();
    }

    // ✅ RESPONDER
    enviar_contexto(fd, ctx, rs_code);
    free(ctx);
}

void cpu_handler_atender_interrupcion(int fd, t_paquete* p)
{
    log_info(logger, "ADAPTER: Interrupción recibida");
    interrupcion_disparar(0);
}
