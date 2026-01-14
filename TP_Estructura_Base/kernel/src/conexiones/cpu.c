#include <conexiones/cpu.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <serializacion/cpu.h>
#include <protocolo/kernel.h>
#include <protocolo/op_code.h>
#include <pthread.h>

// Variables globales para sockets match mod_kernel.c
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern t_log* logger;

static pthread_t hilo_dispatch;
static pthread_t hilo_interrupt;

static void* escuchar_dispatch(void* arg);
static void* escuchar_interrupt(void* arg);

void enviar_contexto_a_cpu(t_contexto_cpu* ctx) {
    // Usamos el helper de protocolo que refactorizamos
    enviar_contexto(socket_cpu_dispatch, ctx, OP_PROCESO_EXEC);
}

void enviar_interrupcion_a_cpu(int pid, int motivo) {
    // TODO: Confirmar si CPU espera payload. Por ahora usamos helper simple.
    enviar_interrupcion_cpu(socket_cpu_interrupt);
}

t_contexto_cpu* recibir_contexto_de_cpu(void) {
    // Funcion no utilizada en modelo asincrono con hilo dedicado
    return NULL; 
}

void conectar_cpu(char* ip, char* puerto_dispatch, char* puerto_interrupt)
{
    // 1. Dispatch
    socket_cpu_dispatch = crear_conexion(ip, puerto_dispatch);
    if(socket_cpu_dispatch < 0) {
        log_error(logger, "Error conectando a CPU dispatch");
        exit(EXIT_FAILURE);
    }
    // Handshake
    handshake_cliente(socket_cpu_dispatch, OP_HANDSHAKE_KERNEL, OP_OK, logger);

    // 2. Interrupt
    socket_cpu_interrupt = crear_conexion(ip, puerto_interrupt);
    if(socket_cpu_interrupt < 0) {
        log_error(logger, "Error conectando a CPU interrupt");
        exit(EXIT_FAILURE);
    }
    // Handshake
    handshake_cliente(socket_cpu_interrupt, OP_HANDSHAKE_KERNEL, OP_OK, logger);

    // 3. Threads
    pthread_create(&hilo_dispatch, NULL, escuchar_dispatch, NULL);
    pthread_create(&hilo_interrupt, NULL, escuchar_interrupt, NULL);

    log_info(logger, "CPU conectada (dispatch + interrupt)");
}

static void* escuchar_dispatch(void* _)
{
    while (1) {
        t_paquete* paquete = recibir_paquete(socket_cpu_dispatch);
        if (paquete == NULL) {
            log_error(logger, "CPU Dispatch desconectado");
            break;
        }

        switch (paquete->codigo_operacion) {

        case OP_FIN_DE_QUANTUM: {
            t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
            log_info(logger, "Fin de Quantum: PID %d", ctx->pid);
            // manejar_fin_quantum(ctx);
            free(ctx);
            break;
        }

        case OP_FIN_DE_PROCESO: {
            // Asumiendo que devuelve contexto o solo PID?
            // Generalmente devuelve contexto actualizado
            t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
            log_info(logger, "Fin de Proceso: PID %d", ctx->pid);
            // manejar_fin_proceso(ctx);
            free(ctx);
            break;
        }

        case OP_BLOQUEO_IO: {
            t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
            log_info(logger, "Bloqueo IO: PID %d. Param: %s", ctx->pid, ctx->parametros);
            manejar_bloqueo_io(ctx);
            free(ctx);
            break;
        }

        case OP_WAIT_RECURSO: {
            t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
            log_info(logger, "WAIT Recurso: %s PID %d", ctx->parametros, ctx->pid);
            manejar_wait_recurso(ctx);
            free(ctx);
            break;
        }

        case OP_SIGNAL_RECURSO: {
            t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
            log_info(logger, "SIGNAL Recurso: %s PID %d", ctx->parametros, ctx->pid);
            manejar_signal_recurso(ctx);
            free(ctx);
            break;
        }

        case OP_SEGFAULT: {
            t_contexto_cpu* ctx = deserializar_contexto_cpu(paquete);
            log_error(logger, "Segfault: PID %d", ctx->pid);
            // manejar_segfault(ctx); // todavia no impl
            free(ctx);
            break;
        }

        default:
            log_warning(logger, "OpCode desconocido CPU->Kernel: %d", paquete->codigo_operacion);
            break;
        }

        paquete_destroy(paquete);
    }

    return NULL;
}

static void* escuchar_interrupt(void* _)
{
    while (1) {
        t_paquete* paquete = recibir_paquete(socket_cpu_interrupt);
        if (paquete == NULL) {
            log_error(logger, "CPU Interrupt desconectado");
            break;
        }

        // Kernel NO deberia recibir nada por interrupt channel, salvo quizas ACKs?
        log_warning(logger, "Kernel recibió algo por interrupt (ignorado): %d", paquete->codigo_operacion);
        
        paquete_destroy(paquete);
    }

    return NULL;
}