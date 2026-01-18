#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <commons/log.h>
#include <paquete/paquete.h>
#include <serializacion/serializacion.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <adaptadores/contexto_cpu_adapter.h>

#include <cpu.h>   // logger, config
#include <conexion/conexion.h>
#include <conexiones/cpu_kernel.h>
#include <ciclo_instruccion/ciclo.h>
#include <instrucciones/instrucciones.h> // For INST_ constants
#include <interrupciones/interrupciones.h>

extern int socket_dispatch;
extern int socket_interrupt;

/* ================= Handlers ================= */

static void* handler_dispatch(void* arg);
static void* handler_interrupt(void* arg);

/* ================= Server launcher ================= */

typedef struct {
    char* puerto;
    char* nombre;
    void* (*handler)(void*);
} t_server_args;

static void free_cpu(t_server_args* args);

static void* thread_server_runner(void* arg)
{
    t_server_args* args = arg;

    int server_fd = iniciar_servidor(args->puerto);
    if (server_fd < 0) {
        log_error(loggerError, "Fallo iniciar server %s", args->nombre);
        free_cpu(args);
    }

    log_info(logger, "Servidor %s escuchando en %s", args->nombre, args->puerto);
    server_escuchar(logger, args->nombre, server_fd, args->handler);

    return NULL;
}

static void free_cpu(t_server_args* args) {
    free(args->nombre);
    free(args->puerto);
    free((void*)args);
}

static void cpu_launch_server(char* puerto, char* nombre, void* (*handler)(void*))
{
    pthread_t thread;
    t_server_args* args = malloc(sizeof(t_server_args));

    args->puerto  = strdup(puerto);
    args->nombre  = strdup(nombre);
    args->handler = handler;

    pthread_create(&thread, NULL, thread_server_runner, args);
    pthread_detach(thread);
}

/* ================= Init ================= */

void cpu_servidores_kernel_init(char* puerto_dispatch, char* puerto_interrupt)
{
    cpu_launch_server(puerto_dispatch, "CPU_DISPATCH", handler_dispatch);
    cpu_launch_server(puerto_interrupt, "CPU_INTERRUPT", handler_interrupt);
}

/* ================= DISPATCH ================= */

static void* handler_dispatch(void* arg)
{
    int fd = *(int*)arg;
    free(arg);

    socket_dispatch = fd;

    log_info(logger, "Kernel conectado a CPU DISPATCH (fd=%d)", fd);

    while (1) {
        t_paquete* paquete = recibir_paquete(fd);
        if (paquete == NULL) break;

        switch (paquete->codigo_operacion) {
            case OP_HANDSHAKE:
                log_info(logger, "Handshake recibido en DISPATCH");
                handshake_servidor(fd, OP_OK, logger);
                break;
            case OP_PROCESO_EXEC:
                cpu_handler_atender_ejecucion(fd, paquete);
                break;
            default:
                log_warning(logger, "CPU Dispatch: OpCode inválido: %d", paquete->codigo_operacion);
                break;
        }

        paquete_destroy(paquete);
    }

    close(fd);
    return NULL;
}

static void* handler_interrupt(void* arg)
{
    int fd = *(int*)arg;
    free(arg);

    socket_interrupt = fd;

    while (1) {
        t_paquete* paquete = recibir_paquete(fd);
        if (paquete == NULL) break;

        switch (paquete->codigo_operacion) {
            case OP_HANDSHAKE:
                log_info(logger, "Handshake recibido en INTERRUPT");
                handshake_servidor(fd, OP_OK, logger);
                break;
            case OP_INTERRUPCION_CPU:
                //cpu_handler_atender_interrupcion(fd, paquete);
                break;
            default:
                log_warning(logger, "CPU Interrupt: OpCode inválido: %d", paquete->codigo_operacion);
                break;
        }

        paquete_destroy(paquete);
    }

    close(fd);
    return NULL;
}


bool kernel_recibir_contexto(t_contexto_cpu* ctx)
{
    t_paquete* p = recibir_paquete(socket_dispatch);
    if (!p) return false;

    if (p->codigo_operacion != OP_PROCESO_EXEC) {
        paquete_destroy(p);
        return false;
    }

    t_contexto_cpu* recibido = recibir_contexto(p);
    paquete_destroy(p);

    //adaptar_contexto_kernel_a_cpu(ctx, recibido);
    free(recibido);

    log_info(logger, "CPU recibió contexto PID=%d", ctx->pid);
    return true;
}

void kernel_enviar_contexto(t_contexto_cpu* ctx, t_motivo_desalojo motivo)
{
    enviar_contexto(socket_dispatch, ctx, OP_PROCESO_EXEC);

    log_info(logger,
        "CPU envió contexto PID=%d MOTIVO=%d",
        ctx->pid,
        motivo
    );
}
