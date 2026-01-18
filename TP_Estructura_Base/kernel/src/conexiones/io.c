#include <conexiones/io.h>
#include <mod_kernel.h>
#include <server/server.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static int socket_server_io = -1;
static t_list* lista_interfaces = NULL;

typedef struct {
    char* nombre;
    int socket;
} t_interfaz_io;

static void* handler_io_connection(void* arg);
static bool _find_interfaz_por_nombre(void* elemento, void* criterio);

void server_io_init(char* puerto) {
    socket_server_io = iniciar_servidor(puerto);
    if (socket_server_io < 0) {
        log_error(loggerError, "Fallo iniciar servidor Kernel para IO");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Server IO iniciado en puerto %s", puerto);

    lista_interfaces = list_create();

    // Lanzar hilo de escucha
    pthread_t th;
    pthread_create(&th, NULL, (void*)server_listen_loop_io, NULL);
    pthread_detach(th);
}

void* server_listen_loop_io(void* arg) {
    server_escuchar(logger, "KERNEL_IO_SERVER", socket_server_io, handler_io_connection);
    return NULL;
}

/**
 * obtener_socket_interfaz
 * Busca una interfaz IO por nombre y retorna su socket.
 * Usada por kernel_io_adapter para enviar requests.
 */
int obtener_socket_interfaz(const char* nombre_interfaz)
{
    if (!nombre_interfaz || !lista_interfaces) {
        return -1;
    }
    // TODO: FIX
    t_interfaz_io* iface = NULL;
    /*t_interfaz_io* iface = list_find(lista_interfaces, 
                                     _find_interfaz_por_nombre, 
                                     (void*)nombre_interfaz);
    */
    if (iface) {
        log_info(logger, "Socket IO encontrado para: %s (FD=%d)", 
                nombre_interfaz, iface->socket);
        return iface->socket;
    }

    log_warning(logger, "Interfaz IO no encontrada: %s", nombre_interfaz);
    return -1;
}

static bool _find_interfaz_por_nombre(void* elemento, void* criterio)
{
    t_interfaz_io* iface = (t_interfaz_io*)elemento;
    const char* nombre = (const char*)criterio;
    
    return strcmp(iface->nombre, nombre) == 0;
}

static void* handler_io_connection(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    // Esperamos Handshake con nombre de interfaz
    t_paquete* p = recibir_paquete(fd);
    if (!p) {
        close(fd);
        return NULL;
    }

    if (p->codigo_operacion == OP_HANDSHAKE) {
        // Leer nombre de la interfaz
        char* nombre = paquete_read_string(p);
        log_info(logger, "Nueva interfaz IO conectada: %s (FD: %d)", nombre, fd);
        
        t_interfaz_io* io = malloc(sizeof(t_interfaz_io));
        io->nombre = nombre;
        io->socket = fd;
        list_add(lista_interfaces, io);
    } else {
        log_warning(logger, "Handshake inválido de IO");
    }
    paquete_destroy(p);

        // Loop de atención a esta interfaz
        while(1) {
            t_paquete* msg = recibir_paquete(fd);
            if (!msg) break;

            switch(msg->codigo_operacion) {
                case OP_IO_FIN_OPERACION:
                    kernel_io_adapter_atender_fin_operacion(fd, msg);
                    break;
                default:
                    log_warning(logger, "Mensaje desconocido de IO: %d", msg->codigo_operacion);
                    break;
            }

            paquete_destroy(msg);
        }

    // Limpieza
    log_info(logger, "Interfaz IO desconectada (FD=%d)", fd);
    close(fd);
    return NULL;
}

