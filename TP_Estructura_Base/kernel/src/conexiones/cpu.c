#include <conexiones/cpu.h>
#include <conexiones/cpu_handlers.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <serializacion/serializacion.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <peticiones/interrupciones.h>
#include <peticiones/dispatch.h>
#include <planificacion/planificacion.h>
#include <adaptadores/kernel_io_adapter.h>
#include <conexiones/memoria.h>
#include <conexiones/io.h>
#include <pthread.h>
#include <pcb/pcb.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <stdlib.h>
#include <string.h>

// Variables globales para sockets match mod_kernel.c
extern int socket_dispatch;
extern int socket_interrupt;
extern t_log* logger;
extern t_log* loggerError;

extern t_list* cola_exit;
extern t_list* cola_exec;
extern pthread_mutex_t mutex_exec;

static pthread_t hilo_interrupt;

static void* escuchar_interrupt(void* arg);

void enviar_contexto_a_cpu(t_contexto_cpu* ctx) {
    // Usamos el helper de protocolo que refactorizamos
    enviar_contexto(socket_dispatch, ctx, OP_PROCESO_EXEC);
}

void enviar_interrupcion_a_cpu(int pid, int motivo) {
    // TODO: Confirmar si CPU espera payload. Por ahora usamos helper simple.
    enviar_interrupcion_cpu(socket_interrupt);
}

void conectar_cpu(char* ip, char* puerto_dispatch, char* puerto_interrupt)
{
    // 1. Dispatch
    socket_dispatch = crear_conexion(ip, puerto_dispatch);
    if(socket_dispatch < 0) {
        log_error(loggerError, "CPU dispatch: error de conexion");
        exit(EXIT_FAILURE);
    }
    // Handshake
    handshake_cliente(socket_dispatch, OP_HANDSHAKE, OP_OK, logger);

    // 2. Interrupt
    socket_interrupt = crear_conexion(ip, puerto_interrupt);
    if(socket_interrupt < 0) {
        log_error(loggerError, "CPU interrupt: error de conexion");
        exit(EXIT_FAILURE);
    }
    // Handshake
    handshake_cliente(socket_interrupt, OP_HANDSHAKE, OP_OK, logger);

    // 3. Threads
    pthread_create(&hilo_interrupt, NULL, escuchar_interrupt, NULL);

    log_info(logger, "CPU: conectada (dispatch + interrupt)");
}

// La función escuchar_dispatch ahora es llamada sincrónicamente por el planificador de corto plazo
void atender_dispatch_cpu(void)
{
    t_paquete* paquete = recibir_paquete(socket_dispatch);
    if (paquete == NULL) {
        log_error(loggerError, "CPU dispatch: desconectado");
        return;
    }

    switch (paquete->codigo_operacion) {

    case OP_FIN_DE_QUANTUM: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_fin_quantum(ctx);
            free(ctx);
        }
        break;
    }

    case OP_CPU_FIN_PROCESO: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            manejar_fin_proceso(ctx);
            free(ctx);
        }
        break;
    }

    case OP_IO_SLEEP: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        char* interfaz = paquete_read_string(paquete);
        uint32_t tiempo = 0;
        paquete_read_uint32(paquete, &tiempo);
        if (ctx) {
            // 1. Buscar PCB y actualizar contexto
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

            // 2. Enviar operacion a interfaz IO
            if (pcb && interfaz) {
                kernel_sleep(pcb, tiempo, interfaz);
            } else {
                log_error(loggerError, "PID: %u - IO_SLEEP error: PCB o interfaz NULL", ctx->pid);
            }

            // 3. Bloquear proceso
            manejar_bloqueo_io(ctx);
            free(ctx);
        }
        if (interfaz) free(interfaz);
        break;
    }

    case OP_IO_STDIN_READ: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            cpu_handler_io_stdin_read(ctx);
            free(ctx);
        }
        break;
    }

    case OP_IO_STDOUT_WRITE: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            cpu_handler_io_stdout_write(ctx);
            free(ctx);
        }
        break;
    }

    case OP_WAIT_RECURSO: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        char* nombre_recurso = paquete_read_string(paquete);
        if (ctx) {
            manejar_wait_recurso(ctx, nombre_recurso);
            free(ctx);
        }
        if (nombre_recurso) free(nombre_recurso);
        break;
    }

    case OP_SIGNAL_RECURSO: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        char* nombre_recurso = paquete_read_string(paquete);
        if (ctx) {
            manejar_signal_recurso(ctx, nombre_recurso);
            free(ctx);
        }
        if (nombre_recurso) free(nombre_recurso);
        break;
    }

    case OP_SEGFAULT: {
        t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
        if (ctx) {
            cpu_handler_segfault(ctx);
            free(ctx);
        }
        break;
    }

    default:
        log_warning(logger, "CPU dispatch: opcode desconocido %d", paquete->codigo_operacion);
        break;
    }

    paquete_destroy(paquete);
}

static void* escuchar_interrupt(void* _)
{
    while (1) {
        t_paquete* paquete = recibir_paquete(socket_interrupt);
        if (paquete == NULL) {
            log_error(loggerError, "CPU interrupt: desconectado");
            break;
        }

        // Kernel NO deberia recibir nada por interrupt channel, salvo quizas ACKs?
        log_warning(logger, "CPU interrupt: mensaje ignorado opcode=%d", paquete->codigo_operacion);

        paquete_destroy(paquete);
    }

    return NULL;
}
