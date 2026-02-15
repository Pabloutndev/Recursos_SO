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
#include <ciclo_instruccion/ciclo.h>
#include <interrupciones/interrupciones.h>
#include <instrucciones/instrucciones.h>
#include <string.h>

extern t_contexto_cpu cpu_estado;
extern t_log* logger;
extern int socket_dispatch;
extern instruccion_t ultima_instruccion;

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
    op_code rs_code;
    switch (motivo) {
        case MOTIVO_EXIT:     rs_code = OP_CPU_FIN_PROCESO; break;
        case MOTIVO_IO:       rs_code = OP_BLOQUEO_IO; break;
        case MOTIVO_SEGFAULT: rs_code = OP_SEGFAULT; break;
        default:              rs_code = OP_DESALOJO; break;
    }

    enviar_contexto(socket_dispatch, ctx, rs_code);

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
    CPU_CTX.motivo_desalojo = CPU_CONTINUAR;
    ciclo_instruccion_ejecutar(ctx);

    // ✅ DETERMINAR RESPUESTA
    op_code rs_code;

    if (CPU_CTX.motivo_desalojo == MOTIVO_EXIT) {
        rs_code = OP_CPU_FIN_PROCESO;
    } else if (CPU_CTX.motivo_desalojo == MOTIVO_IO) {
        // Mapeo detallado basado en la instrucción que causó el desalojo
        switch (ultima_instruccion.opcode) {
            case INST_WAIT:           rs_code = OP_WAIT_RECURSO; break;
            case INST_SIGNAL:         rs_code = OP_SIGNAL_RECURSO; break;
            case INST_IO_GEN_SLEEP:    rs_code = OP_IO_SLEEP; break;
            case INST_IO_STDIN_READ:  rs_code = OP_IO_STDIN_READ; break;
            case INST_IO_STDOUT_WRITE:rs_code = OP_IO_STDOUT_WRITE; break;
            case INST_IO_FS_CREATE:   rs_code = OP_IO_FS_CREATE; break;
            case INST_IO_FS_DELETE:   rs_code = OP_IO_FS_DELETE; break;
            case INST_IO_FS_TRUNCATE: rs_code = OP_IO_FS_TRUNCATE; break;
            case INST_IO_FS_WRITE:    rs_code = OP_IO_FS_WRITE; break;
            case INST_IO_FS_READ:     rs_code = OP_IO_FS_READ; break;
            default:                  rs_code = OP_BLOQUEO_IO; break;
        }
    } else if (CPU_CTX.motivo_desalojo == MOTIVO_SEGFAULT) {
        rs_code = OP_SEGFAULT;
    } else if (interrupcion_pendiente()) {
        rs_code = OP_FIN_DE_QUANTUM;
        interrupcion_reset();
    } else {
        rs_code = OP_DESALOJO;
    }

    const char* motivo_str;
    switch(rs_code) {
        case OP_CPU_FIN_PROCESO: motivo_str = "EXIT"; break;
        case OP_FIN_DE_QUANTUM: motivo_str = "FIN_QUANTUM"; break;
        case OP_SEGFAULT: motivo_str = "SEGFAULT"; break;
        case OP_IO_SLEEP: motivo_str = "IO_SLEEP"; break;
        case OP_IO_STDIN_READ: motivo_str = "IO_STDIN"; break;
        case OP_IO_STDOUT_WRITE: motivo_str = "IO_STDOUT"; break;
        case OP_WAIT_RECURSO: motivo_str = "WAIT"; break;
        case OP_SIGNAL_RECURSO: motivo_str = "SIGNAL"; break;
        default: motivo_str = "DESALOJO"; break;
    }
    log_info(logger, "PID: %u - Desalojo: %s", ctx->pid, motivo_str);

    // ✅ RESPONDER
    if (rs_code == OP_WAIT_RECURSO || rs_code == OP_SIGNAL_RECURSO) {
        // Para WAIT/SIGNAL: enviar contexto + nombre del recurso
        t_paquete* pkt = serializar_contexto_cpu(ctx, rs_code);
        paquete_write_string(pkt, ultima_instruccion.parametros);
        enviar_paquete(fd, pkt);
        paquete_destroy(pkt);
    } else {
        enviar_contexto(fd, ctx, rs_code);
    }
    free(ctx);
}

void cpu_handler_atender_interrupcion(int fd, t_paquete* p)
{
    log_info(logger, "ADAPTER: Interrupción recibida");
    interrupcion_disparar(0);
}
