#include <conexiones/io.h>
#include <mod_kernel.h>
#include <adaptadores/kernel_io_adapter.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

static int socket_server_io = -1;
static t_list* lista_interfaces = NULL;
static pthread_mutex_t mutex_interfaces;

typedef struct {
    char* nombre;
    int socket;
} t_interfaz_io;

/* Variables de búsqueda para closures de commons (no aceptan args extra) */
static const char* _buscar_nombre = NULL;
static int _buscar_socket = -1;

static void* handler_io_connection(void* arg);

void server_io_init(char* puerto) {
    socket_server_io = iniciar_servidor(puerto);
    if (socket_server_io < 0) {
        log_error(loggerError, "Fallo iniciar servidor Kernel para IO");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Server IO iniciado en puerto %s", puerto);

    lista_interfaces = list_create();
    pthread_mutex_init(&mutex_interfaces, NULL);
}

/* Closures para list_find / list_remove_and_destroy_by_condition */
static bool _match_por_nombre(void* elemento) {
    t_interfaz_io* iface = (t_interfaz_io*)elemento;
    return strcmp(iface->nombre, _buscar_nombre) == 0;
}

static bool _match_por_socket(void* elemento) {
    t_interfaz_io* iface = (t_interfaz_io*)elemento;
    return iface->socket == _buscar_socket;
}

static void _destroy_interfaz(void* data) {
    t_interfaz_io* iface = (t_interfaz_io*)data;
    free(iface->nombre);
    free(iface);
}

int obtener_socket_interfaz(const char* nombre_interfaz)
{
    if (!nombre_interfaz || !lista_interfaces) {
        return -1;
    }

    pthread_mutex_lock(&mutex_interfaces);
    _buscar_nombre = nombre_interfaz;
    t_interfaz_io* iface = list_find(lista_interfaces, _match_por_nombre);
    pthread_mutex_unlock(&mutex_interfaces);

    if (iface) {
        log_info(logger, "Socket IO encontrado para: %s (FD=%d)",
                nombre_interfaz, iface->socket);
        return iface->socket;
    }

    log_warning(logger, "Interfaz IO no encontrada: %s", nombre_interfaz);
    return -1;
}

void* handler_io_connection(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    // Esperamos Handshake con nombre de interfaz
    t_paquete* p = recibir_paquete(fd);
    if (!p) {
        close(fd);
        return NULL;
    }

    if (p->codigo_operacion == OP_HANDSHAKE_IO) {
        char* nombre = paquete_read_string(p);
        log_info(logger, "Nueva interfaz IO conectada: %s (FD: %d)", nombre, fd);

        t_interfaz_io* io = malloc(sizeof(t_interfaz_io));
        io->nombre = strdup(nombre);
        io->socket = fd;

        pthread_mutex_lock(&mutex_interfaces);
        list_add(lista_interfaces, io);
        pthread_mutex_unlock(&mutex_interfaces);
        free(nombre);
    } else {
        log_warning(logger, "Handshake inválido de IO");
    }
    paquete_destroy(p);

    // Responder OK al handshake
    enviar_respuesta_ok(fd);

    // Loop de atención a esta interfaz
    while (1) {
        t_paquete* msg = recibir_paquete(fd);
        if (!msg) break;

        switch (msg->codigo_operacion) {
            case OP_IO_FIN_OPERACION:
                kernel_io_adapter_atender_fin_operacion(fd, msg);
                break;
            default:
                log_warning(logger, "Mensaje desconocido de IO: %d", msg->codigo_operacion);
                break;
        }

        paquete_destroy(msg);
    }

    // Limpieza: remover interfaz de la lista
    log_info(logger, "Interfaz IO desconectada (FD=%d)", fd);

    pthread_mutex_lock(&mutex_interfaces);
    _buscar_socket = fd;
    list_remove_and_destroy_by_condition(lista_interfaces, _match_por_socket, _destroy_interfaz);
    pthread_mutex_unlock(&mutex_interfaces);

    close(fd);
    return NULL;
}
