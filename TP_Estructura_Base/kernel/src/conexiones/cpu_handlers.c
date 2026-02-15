#include <conexiones/cpu_handlers.h>
#include <mod_kernel.h>
#include <peticiones/interrupciones.h>
#include <planificacion/planificacion.h>
#include <conexiones/memoria.h>
#include <conexiones/io.h>
#include <pcb/pcb.h>
#include <protocolo/mensajes.h>
#include <commons/log.h>
#include <string.h>

extern t_log* logger;
extern t_log* loggerError;

extern t_list* cola_exec;
extern pthread_mutex_t mutex_exec;

void cpu_handler_segfault(t_contexto_cpu* ctx)
{
    if (!ctx) return;

    log_error(loggerError, "PID: %d - Segmentation fault PC=%u", ctx->pid, ctx->pc);

    // Actualizar PC en PCB
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* pcb = (t_pcb*) list_get(cola_exec, i);
        if (pcb && pcb->pid == (uint32_t)ctx->pid) {
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    // Finalizar por segfault
    manejar_interrupcion(ctx->pid, "EXIT");

    // Liberar recursos
    solicitar_fin_proceso_memoria(ctx->pid);
}

void cpu_handler_io_stdin_read(t_contexto_cpu* ctx)
{
    if (!ctx) return;

    log_info(logger, "PID: %u - IO_STDIN_READ PC=%u", ctx->pid, ctx->pc);

    // 1. Buscar PCB en EXEC y actualizar contexto
    t_pcb* pcb = NULL;
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* p = (t_pcb*) list_get(cola_exec, i);
        if (p && p->pid == (uint32_t)ctx->pid) {
            pcb = p;
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    if (!pcb) {
        log_error(loggerError, "PID: %u - IO_STDIN_READ error: PCB no encontrado", ctx->pid);
        return;
    }

    // 2. Construir request de IO STDIN con datos de registros
    //    Convención TP: dirección lógica en EAX, tamaño en EBX,
    //    nombre de interfaz se pasa como string desde la instrucción.
    //    Como el contexto no trae el nombre, usamos un genérico "STDIN".
    t_io_stdin_read req;
    req.pid = ctx->pid;
    req.direccion_logica = ctx->registros.EAX;
    req.size = ctx->registros.EBX;
    strncpy(req.interfaz, "STDIN", sizeof(req.interfaz) - 1);
    req.interfaz[sizeof(req.interfaz) - 1] = '\0';

    // 3. Buscar interfaz y enviar operación
    int socket_io = obtener_socket_interfaz(req.interfaz);
    if (socket_io >= 0) {
        enviar_io_stdin_read(socket_io, &req);
        log_info(logger, "PID: %u - IO_STDIN_READ dir=%u size=%u -> %s",
                 req.pid, req.direccion_logica, req.size, req.interfaz);
    } else {
        log_error(loggerError, "PID: %u - IO_STDIN_READ error: interfaz '%s' no encontrada",
                  req.pid, req.interfaz);
    }

    // 4. Bloquear proceso (EXEC -> BLOCKED)
    manejar_interrupcion(ctx->pid, "IO");
}

void cpu_handler_io_stdout_write(t_contexto_cpu* ctx)
{
    if (!ctx) return;

    log_info(logger, "PID: %u - IO_STDOUT_WRITE PC=%u", ctx->pid, ctx->pc);

    // 1. Buscar PCB en EXEC y actualizar contexto
    t_pcb* pcb = NULL;
    pthread_mutex_lock(&mutex_exec);
    for (int i = 0; i < list_size(cola_exec); i++) {
        t_pcb* p = (t_pcb*) list_get(cola_exec, i);
        if (p && p->pid == (uint32_t)ctx->pid) {
            pcb = p;
            pcb->program_counter = ctx->pc;
            pcb->registros = ctx->registros;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_exec);

    if (!pcb) {
        log_error(loggerError, "PID: %u - IO_STDOUT_WRITE error: PCB no encontrado", ctx->pid);
        return;
    }

    // 2. Construir request de IO STDOUT con datos de registros
    t_io_stdout_write req;
    req.pid = ctx->pid;
    req.direccion_logica = ctx->registros.EAX;
    req.size = ctx->registros.EBX;
    strncpy(req.interfaz, "STDOUT", sizeof(req.interfaz) - 1);
    req.interfaz[sizeof(req.interfaz) - 1] = '\0';

    // 3. Buscar interfaz y enviar operación
    int socket_io = obtener_socket_interfaz(req.interfaz);
    if (socket_io >= 0) {
        enviar_io_stdout_write(socket_io, &req);
        log_info(logger, "PID: %u - IO_STDOUT_WRITE dir=%u size=%u -> %s",
                 req.pid, req.direccion_logica, req.size, req.interfaz);
    } else {
        log_error(loggerError, "PID: %u - IO_STDOUT_WRITE error: interfaz '%s' no encontrada",
                  req.pid, req.interfaz);
    }

    // 4. Bloquear proceso (EXEC -> BLOCKED)
    manejar_interrupcion(ctx->pid, "IO");
}
