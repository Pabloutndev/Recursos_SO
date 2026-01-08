#include <conexiones/cpu_kernel.h>
#include <server/server.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <cpu.h>   // logger, config
#include <ciclo/ciclo_instruccion.h>

#include <utils/protocolo/protocolo_kernel_cpu.h>

/* ================= Estado CPU ================= */

static volatile bool interrupcion_pendiente = false;

/* ================= Handlers ================= */

static void* handler_dispatch(void* arg);
static void* handler_interrupt(void* arg);

/* ================= Server launcher ================= */

typedef struct {
    char* puerto;
    char* nombre;
    void* (*handler)(void*);
} t_server_args;

static void* thread_server_runner(void* arg)
{
    t_server_args* args = arg;

    int server_fd = iniciar_servidor(args->puerto);
    if (server_fd < 0) {
        log_error(loggerError, "Fallo iniciar server %s", args->nombre);
        goto cleanup;
    }

    log_info(logger, "Servidor %s escuchando en %s", args->nombre, args->puerto);
    server_escuchar(logger, args->nombre, server_fd, args->handler);

cleanup:
    free(args->nombre);
    free(args->puerto);
    free(args);
    return NULL;
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

    log_info(logger, "Kernel conectado a CPU DISPATCH (fd=%d)", fd);

    while (1) {

        op_code op = recibir_operacion(fd);
        if (op < 0) {
            log_warning(logger, "Kernel Dispatch desconectado");
            break;
        }

        switch (op) {

        case PROCESO_EXEC: {
            t_contexto_cpu* ctx =
                protocolo_cpu_recibir_exec(fd);

            log_info(logger, "Ejecutando PID %u", ctx->pid);

            ciclo_instruccion_ejecutar(ctx, &interrupcion_pendiente);

            free(ctx);
            break;
        }

        default:
            log_warning(logger, "OpCode invalido en DISPATCH: %d", op);
            break;
        }
    }

    close(fd);
    return NULL;
}

/* ================= INTERRUPT ================= */

static void* handler_interrupt(void* arg)
{
    int fd = *(int*)arg;
    free(arg);

    log_info(logger, "Kernel conectado a CPU INTERRUPT (fd=%d)", fd);

    while (1) {

        op_code op = recibir_operacion(fd);
        if (op < 0) {
            log_warning(logger, "Kernel Interrupt desconectado");
            break;
        }

        if (op == INTERRUPCION_CPU) {
            log_info(logger, "Interrupcion recibida");
            interrupcion_pendiente = true;
        } else {
            log_warning(logger, "OpCode invalido en INTERRUPT: %d", op);
        }
    }

    close(fd);
    return NULL;
}
