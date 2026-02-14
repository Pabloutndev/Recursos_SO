#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <commons/log.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <adaptadores/contexto_cpu_adapter.h>
#include <cpu.h>
#include <conexion/conexion.h>

extern t_log* logger;
extern t_log* loggerError;
extern int socket_dispatch;
extern int socket_interrupt;

typedef struct {
    char* puerto;
    char* nombre;
    void* (*handler)(void*);
} t_server_args;

static void* thread_server_runner(void* arg) {
    t_server_args* args = arg;
    int server_fd = iniciar_servidor(args->puerto);
    
    if (server_fd < 0) {
        log_error(loggerError, "Fallo iniciar server %s en puerto %s", args->nombre, args->puerto);
        free(args->nombre);
        free(args->puerto);
        free(args);
        return NULL;
    }

    log_info(logger, "Servidor %s escuchando en puerto %s", args->nombre, args->puerto);
    server_escuchar(logger, args->nombre, server_fd, args->handler);
    
    close(server_fd);
    return NULL;
}

static void* handler_dispatch(void* arg) {
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
                // Delegamos al adapter para atender la ejecucion
                cpu_handler_atender_ejecucion(fd, paquete);
                break;
            default:
                log_warning(logger, "CPU Dispatch: OP_CODE desconocido: %d", paquete->codigo_operacion);
                break;
        }

        paquete_destroy(paquete);
    }

    log_warning(logger, "Conexion DISPATCH cerrada (fd=%d)", fd);
    close(fd);
    return NULL;
}

static void* handler_interrupt(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    socket_interrupt = fd;
    log_info(logger, "Kernel conectado a CPU INTERRUPT (fd=%d)", fd);

    while (1) {
        t_paquete* paquete = recibir_paquete(fd);
        if (paquete == NULL) break;

        switch (paquete->codigo_operacion) {
            case OP_HANDSHAKE:
                log_info(logger, "Handshake recibido en INTERRUPT");
                handshake_servidor(fd, OP_OK, logger);
                break;
            case OP_INTERRUPCION_CPU:
                cpu_handler_atender_interrupcion(fd, paquete);
                break;
            default:
                log_warning(logger, "CPU Interrupt: OP_CODE desconocido: %d", paquete->codigo_operacion);
                break;
        }

        paquete_destroy(paquete);
    }

    log_warning(logger, "Conexion INTERRUPT cerrada (fd=%d)", fd);
    close(fd);
    return NULL;
}

void cpu_servidores_kernel_init(char* puerto_dispatch, char* puerto_interrupt) {
    pthread_t th_dispatch, th_interrupt;

    t_server_args* args_d = malloc(sizeof(t_server_args));
    args_d->puerto = strdup(puerto_dispatch);
    args_d->nombre = strdup("CPU_DISPATCH");
    args_d->handler = handler_dispatch;

    t_server_args* args_i = malloc(sizeof(t_server_args));
    args_i->puerto = strdup(puerto_interrupt);
    args_i->nombre = strdup("CPU_INTERRUPT");
    args_i->handler = handler_interrupt;

    pthread_create(&th_dispatch, NULL, thread_server_runner, args_d);
    pthread_create(&th_interrupt, NULL, thread_server_runner, args_i);

    pthread_detach(th_dispatch);
    pthread_detach(th_interrupt);
}
