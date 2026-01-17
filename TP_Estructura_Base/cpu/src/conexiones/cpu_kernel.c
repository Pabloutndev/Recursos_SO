#include <conexiones/cpu_kernel.h>
#include <server/server.h>
#include <commons/log.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <cpu.h>   // logger, config
#include <ciclo_instruccion/ciclo.h>
#include <paquete/paquete.h>
#include <protocolo/mensajes.h>
#include <interrupciones/interrupciones.h>
#include <protocolo/op_code.h>
#include <serializacion/serializacion.h>
#include <instrucciones/instrucciones.h> // For INST_ constants

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

    log_info(logger, "Kernel conectado a CPU DISPATCH (fd=%d)", fd);

    while (1) {
        t_paquete* paquete = recibir_paquete(fd);
        
        if (paquete == NULL) {
            log_warning(logger, "Kernel Dispatch desconectado");
            break;
        }

        switch (paquete->codigo_operacion) {

        case OP_PROCESO_EXEC: {
            t_contexto_cpu* ctx = recibir_contexto(paquete);
            
            if (!ctx) {
                log_error(logger, "Error deserializando contexto");
                break;
            }
            
            log_info(logger, "CPU Dispatch: Ejecutando PID %u, PC=%u, Quantum=%u", 
                     ctx->pid, ctx->pc, ctx->quantum);

            // ✅ CICLO PRINCIPAL DE CPU
            ciclo_instruccion_ejecutar(ctx);

            // ✅ DETERMINAR MOTIVO DE DEVOLUCIÓN
            op_code rs_code = OP_DESALOJO;  // Default

            if (ctx->finalizado) {
                // Proceso finalizó normalmente (EXIT o EOF)
                rs_code = OP_MEM_FIN_PROCESO;
                log_info(logger, "CPU Dispatch: PID %u finalizó normalmente", ctx->pid);
                
            } else if (interrupcion_pendiente()) {
                // Fue interrumpido por Kernel (quantum vencido)
                rs_code = OP_FIN_DE_QUANTUM;
                interrupcion_reset();  // Limpiar flag
                log_info(logger, "CPU Dispatch: PID %u interrumpido por quantum", ctx->pid);
                
            } else if (ctx->bloqueado) {
                // Proceso se bloqueó (IO, WAIT, SIGNAL, etc.)
                switch(ctx->motivo_desalojo) {
                    case INST_WAIT: 
                        rs_code = OP_WAIT_RECURSO;
                        log_info(logger, "CPU Dispatch: PID %u bloqueado por WAIT", ctx->pid);
                        break;
                        
                    case INST_SIGNAL: 
                        rs_code = OP_SIGNAL_RECURSO;
                        log_info(logger, "CPU Dispatch: PID %u bloqueado por SIGNAL", ctx->pid);
                        break;
                        
                    case INST_IO_GEN_SLEEP: 
                    case INST_IO_STDIN_READ: 
                    case INST_IO_STDOUT_WRITE: 
                    case INST_IO_FS_CREATE: 
                    case INST_IO_FS_DELETE: 
                    case INST_IO_FS_TRUNCATE: 
                    case INST_IO_FS_WRITE: 
                    case INST_IO_FS_READ: 
                        rs_code = OP_BLOQUEO_IO;
                        log_info(logger, "CPU Dispatch: PID %u bloqueado por IO (%u)", 
                                 ctx->pid, ctx->motivo_desalojo);
                        break;
                        
                    default: 
                        rs_code = OP_BLOQUEO_IO;
                        log_warning(logger, "CPU Dispatch: PID %u motivo desalojo desconocido (%u)",
                                    ctx->pid, ctx->motivo_desalojo);
                        break;
                }
            }

            // ✅ ENVIAR RESPUESTA A KERNEL
            enviar_contexto(fd, ctx, rs_code);
            log_info(logger, "CPU Dispatch: Contexto retornado PID %u (Code: %d)", ctx->pid, rs_code);

            free(ctx); 
            break;
        }

        default:
            log_warning(logger, "CPU Dispatch: OpCode inválido: %d", paquete->codigo_operacion);
            break;
        }

        paquete_destroy(paquete);
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
        t_paquete* paquete = recibir_paquete(fd);

        if (paquete == NULL) {
            log_warning(logger, "Kernel Interrupt desconectado");
            break;
        }

        if (paquete->codigo_operacion == OP_INTERRUPCION_CPU) {
            log_info(logger, "Interrupcion recibida de Kernel");
            interrupcion_disparar(0); 
        } else {
            log_warning(logger, "OpCode invalido en INTERRUPT: %d", paquete->codigo_operacion);
        }

        paquete_destroy(paquete);
    }

    close(fd);
    return NULL;
}
