#include <conexiones/memoria.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <serializacion/serializacion.h>
#include <commons/log.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

extern int socket_memoria;
extern t_log* logger;
extern t_log* loggerError;

static pthread_mutex_t mutex_socket_memoria = PTHREAD_MUTEX_INITIALIZER;

void conectar_memoria(char* ip, char* puerto) {
    socket_memoria = crear_conexion(ip, puerto);
    if (socket_memoria < 0) {
        log_error(loggerError, "Memoria: error de conexion");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Memoria: conectada %s:%s", ip, puerto);

    // Handshake
    handshake_cliente(socket_memoria, OP_HANDSHAKE, OP_OK, logger);
}

bool solicitar_creacion_proceso_memoria(uint32_t pid, const char* path)
{
    if (socket_memoria < 0) {
        log_error(loggerError, "Memoria: socket no valido");
        return false;
    }

    if (!path) {
        log_error(loggerError, "Memoria: path de proceso invalido");
        return false;
    }

    // Crear estructura de request
    t_mem_init_proceso req;
    req.pid = pid;
    snprintf(req.path, sizeof(req.path), "%s", path);

    // Enviar a Memoria (protegido por mutex)
    pthread_mutex_lock(&mutex_socket_memoria);

    log_info(logger, "PID: %u - Memoria: solicitud creacion path=%s", pid, path);
    enviar_init_proceso(socket_memoria, &req, OP_MEM_INIT_PROCESO);

    // Esperar respuesta (bloqueante)
    t_paquete* resp = recibir_paquete(socket_memoria);

    pthread_mutex_unlock(&mutex_socket_memoria);

    if (!resp) {
        log_error(loggerError, "Memoria: error recibiendo respuesta");
        return false;
    }

    // Validar respuesta
    bool exito = (resp->codigo_operacion == OP_OK);
    if (exito) {
        log_info(logger, "PID: %u - Memoria: creacion confirmada", pid);
    } else {
        log_error(loggerError, "PID: %u - Memoria: creacion rechazada", pid);
    }

    paquete_destroy(resp);
    return exito;
}

void solicitar_fin_proceso_memoria(uint32_t pid)
{
    if (socket_memoria < 0) {
        log_warning(logger, "Memoria: socket no valido, no se puede notificar fin");
        return;
    }

    // Crear estructura de request
    t_mem_fin_proceso req;
    req.pid = pid;

    // Enviar a Memoria y leer respuesta (protegido por mutex)
    pthread_mutex_lock(&mutex_socket_memoria);

    log_info(logger, "PID: %u - Memoria: solicitud fin proceso", pid);
    enviar_fin_proceso(socket_memoria, &req, OP_MEM_FIN_PROCESO);

    // Leer respuesta para mantener protocolo sincronizado.
    // Memoria siempre responde OP_OK a fin_proceso.
    t_paquete* resp = recibir_paquete(socket_memoria);
    if (resp) {
        paquete_destroy(resp);
    }

    pthread_mutex_unlock(&mutex_socket_memoria);
}
