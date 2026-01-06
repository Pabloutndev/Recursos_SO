
#include <conexiones/cpu_kernel.h>
#include <server/server.h> // Shared utils server
#include <paquete/paquete.h>
#include <pthread.h>
#include <commons/log.h>
#include <cpu.h> // Access to global config/logger

// Handlers
void* handler_dispatch(void* arg);
void* handler_interrupt(void* arg);

typedef struct {
    char* puerto;
    char* nombre;
    void* (*handler)(void*);
} t_server_args;

void* thread_server_runner(void* arg) {
    t_server_args* args = (t_server_args*)arg;
    int socket = iniciar_servidor(args->puerto);
    if (socket < 0) {
        log_error(loggerError, "Fallo iniciar server %s en puerto %s", args->nombre, args->puerto);
    } else {
        server_escuchar(logger, args->nombre, socket, args->handler);
    }
    free(args->nombre);
    free(args->puerto); // strdup assumed
    free(args);
    return NULL;
}

void cpu_launch_server(char* puerto, char* nombre, void* (*handler)(void*)) {
    pthread_t thread;
    t_server_args* args = malloc(sizeof(t_server_args));
    args->puerto = strdup(puerto);
    args->nombre = strdup(nombre);
    args->handler = handler;
    pthread_create(&thread, NULL, thread_server_runner, args);
    pthread_detach(thread);
}

void cpu_servidores_kernel_init(char* puerto_dispatch, char* puerto_interrupt) {
    cpu_launch_server(puerto_dispatch, "CPU_DISPATCH", handler_dispatch);
    cpu_launch_server(puerto_interrupt, "CPU_INTERRUPT", handler_interrupt);
}

// Handler Dispatch: Recibe Contexto -> Ciclo Instruccion
void* handler_dispatch(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    log_info(logger, "Cliente Dispatch conectado FD: %d", fd);

    while(1) {
        int cod_op = recibir_operacion(fd);
        if (cod_op < 0) {
            log_warning(logger, "Kernel Dispatch desconectado");
            break;
        }

        switch(cod_op) {
            case PROCESO_EXEC:
                // Recibir contexto
                // t_list* p = recibir_paquete(fd);
                // deserializar_contexto...
                log_info(logger, "Recibido contexto para ejecucion");
                // ciclo_instruccion_ejecutar(ctx);
                break;
            default:
                log_warning(logger, "OpCode invalido en Dispatch: %d", cod_op);
                break;
        }
    }
    close(fd);
    return NULL;
}

// Handler Interrupt: Recibe Interrupcion -> Flag Ciclo
void* handler_interrupt(void* arg) {
    int fd = *(int*)arg;
    free(arg);
    
    log_info(logger, "Cliente Interrupt conectado FD: %d", fd);

    while(1) {
        int cod_op = recibir_operacion(fd);
        if (cod_op < 0) {
            log_warning(logger, "Kernel Interrupt desconectado");
            break;
        }
        
        switch(cod_op) {
            case INTERRUPCION_CPU:
                log_info(logger, "Recibida Interrupcion");
                // set_flag_interrupcion(true);
                break;
            default:
                 log_warning(logger, "OpCode invalido en Interrupt: %d", cod_op);
                break;
        }
    }
    close(fd);
    return NULL;
}