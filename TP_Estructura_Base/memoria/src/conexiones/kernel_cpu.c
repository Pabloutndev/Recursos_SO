#include <conexiones/kernel_cpu.h>
#include <conexion/conexion.h>
#include <commons/log.h>

#include <utils/protocolo/protocolo_memoria.h>

#include <pthread.h>
#include <stdlib.h>

/* ========= Globals ========= */

extern t_log* logger;

static int socket_kernel = -1;
static int socket_cpu = -1;

/* ========= Hilos ========= */

static pthread_t hilo_kernel;
static pthread_t hilo_cpu;

/* ========= Prototipos ========= */

static void* escuchar_kernel(void* arg);
static void* escuchar_cpu(void* arg);

/* ========= Conexiones ========= */

void iniciar_conexiones_memoria(char* puerto_kernel, char* puerto_cpu)
{
    int server_kernel = iniciar_servidor(puerto_kernel);
    int server_cpu = iniciar_servidor(puerto_cpu);

    socket_kernel = esperar_cliente(server_kernel);
    handshake_servidor(socket_kernel, HANDSHAKE_MEMORIA, OK, logger);

    socket_cpu = esperar_cliente(server_cpu);
    handshake_servidor(socket_cpu, HANDSHAKE_MEMORIA, OK, logger);

    pthread_create(&hilo_kernel, NULL, escuchar_kernel, NULL);
    pthread_create(&hilo_cpu, NULL, escuchar_cpu, NULL);

    log_info(logger, "Kernel y CPU conectados a Memoria");
}

static void* escuchar_kernel(void* _)
{
    while (1) {

        op_code op = recibir_operacion(socket_kernel);
        if (op < 0) break;

        switch (op) {

        case INIT_PROCESO: {
            t_mem_init_proceso* req =
                protocolo_memoria_recibir_init(socket_kernel);

            manejar_init_proceso(req);
            free(req);
            break;
        }

        case FIN_PROCESO: {
            t_mem_fin_proceso* req =
                protocolo_memoria_recibir_fin(socket_kernel);

            manejar_fin_proceso(req);
            free(req);
            break;
        }

        default:
            log_error(logger, "Op desconocido Kernel->Memoria");
            break;
        }
    }
    return NULL;
}


static void* escuchar_cpu(void* _)
{
    while (1) {

        op_code op = recibir_operacion(socket_cpu);
        if (op < 0) break;

        switch (op) {

        case FETCH_INSTRUCCION:
            protocolo_memoria_responder_instruccion(socket_cpu);
            break;

        case LEER_MEMORIA:
            protocolo_memoria_responder_lectura(socket_cpu);
            break;

        case ESCRIBIR_MEMORIA:
            protocolo_memoria_escribir(socket_cpu);
            break;

        default:
            log_error(logger, "Op desconocido CPU->Memoria");
            break;
        }
    }
    return NULL;
}
