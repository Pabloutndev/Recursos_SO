#include <adaptadores/cpu_dispatch_handler.h>
#include <model/model.h>
#include <cpu.h>
#include <stdlib.h>
#include <commons/log.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <paquete/paquete.h>
#include <serializacion/serializacion.h>
#include <conexion/conexion.h>
#include <ciclo_instruccion/ciclo.h>
#include <interrupciones/interrupciones.h>
#include <instrucciones/instrucciones.h>

extern t_log* logger;
extern instruccion_t ultima_instruccion;

void cpu_handler_atender_ejecucion(int fd, t_paquete* p)
{
    t_contexto_cpu* ctx = recibir_contexto(p);
    if (!ctx) return;

    log_info(logger, "PID: %u - Inicio ejecucion", ctx->pid);

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
    } else if (rs_code == OP_IO_SLEEP) {
        // Para IO_SLEEP: enviar contexto + nombre interfaz + tiempo
        t_paquete* pkt = serializar_contexto_cpu(ctx, rs_code);
        paquete_write_string(pkt, ultima_instruccion.parametros);
        paquete_write_uint32(pkt, ultima_instruccion.inmediato);
        enviar_paquete(fd, pkt);
        paquete_destroy(pkt);
    } else {
        enviar_contexto(fd, ctx, rs_code);
    }
    free(ctx);
}

void cpu_handler_atender_interrupcion(int fd, t_paquete* p)
{
    log_info(logger, "Interrupcion recibida");
    interrupcion_disparar(0);
}
