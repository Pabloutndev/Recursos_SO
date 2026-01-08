
#include <conexiones/cpu.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <pthread.h>

// Variables globales para sockets (definidas en mod_kernel.c o aquí)
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern t_log* logger;

static pthread_t hilo_dispatch;
static pthread_t hilo_interrupt;

static void* escuchar_dispatch(void* arg);
static void* escuchar_interrupt(void* arg);

void conectar_cpu(char* ip, char* puerto_dispatch, char* puerto_interrupt)
{
    socket_cpu_dispatch = crear_conexion(ip, puerto_dispatch);

    if(socket_cpu_dispatch < 0) {
        log_error(logger, "Error conectando a CPU dispatch");
        exit(EXIT_FAILURE);
    }

    handshake_cliente(socket_cpu_dispatch, HANDSHAKE_KERNEL, OK, logger);

    socket_cpu_interrupt = crear_conexion(ip, puerto_interrupt);

    if(socket_cpu_interrupt < 0) {
        log_error(logger, "Error conectando a CPU interrupt");
        exit(EXIT_FAILURE);
    }
    
    handshake_cliente(socket_cpu_interrupt, HANDSHAKE_KERNEL, OK, logger);

    pthread_create(&hilo_dispatch, NULL, escuchar_dispatch, NULL);
    pthread_create(&hilo_interrupt, NULL, escuchar_interrupt, NULL);

    log_info(logger, "CPU conectada (dispatch + interrupt)");
}

static void* escuchar_dispatch(void* _)
{
    while (1) {
        op_code op = recibir_operacion(socket_cpu_dispatch);
        if (op < 0) {
            log_error(logger, "CPU Dispatch desconectado");
            break;
        }

        switch (op) {

        case FIN_DE_QUANTUM: 
            t_contexto_cpu* ctx = 
                protocolo_kernel_cpu_recibir_fin_quantum(socket_cpu_dispatch);
            
            manejar_fin_quantum(ctx);
            free(ctx);
            break;

        case FIN_DE_PROCESO:
            t_contexto_cpu* ctx =
                protocolo_kernel_cpu_recibir_fin_proceso(socket_cpu_dispatch);

            manejar_fin_proceso(ctx);
            free(ctx);
            break;

        case BLOQUEO_IO:
            t_contexto_cpu* ctx =
                protocolo_kernel_cpu_recibir_bloqueo_io(socket_cpu_dispatch);

            manejar_bloqueo_io(ctx);
            free(ctx);
            break;

        case SEGFAULT: 
            t_contexto_cpu* ctx =
                protocolo_kernel_cpu_recibir_segfault(socket_cpu_dispatch);

            manejar_segfault(ctx);
            free(ctx);
            break;

        default:
            log_error(logger, "OpCode desconocido CPU->Kernel: %d", op);
            break;
        }
    }

    return NULL;
}

static void* escuchar_interrupt(void* _)
{
    while (1) {
        op_code op = recibir_operacion(socket_cpu_interrupt);
        if (op < 0) {
            log_error(logger, "CPU Interrupt desconectado");
            break;
        }

        // Kernel NO recibe nada por interrupt
        // Este socket existe SOLO para enviar interrupciones
        log_warning(logger, "Kernel recibió algo por interrupt (ignorado)");
    }

    return NULL;
}