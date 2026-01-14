#include <conexiones/io.h>
#include <mod_kernel.h>
#include <server/server.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <pthread.h>

static int socket_server_io = -1;
static t_list* lista_interfaces = NULL;

typedef struct {
    char* nombre;
    int socket;
    // Tipos de operaciones soportadas?
} t_interfaz_io;

static void* handler_io_connection(void* arg);

void server_io_init(char* puerto) {
    socket_server_io = iniciar_servidor(puerto);
    if (socket_server_io < 0) {
        log_error(loggerError, "Fallo iniciar servidor Kernel para IO");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Server IO iniciado en puerto %s", puerto);

    lista_interfaces = list_create();

    // Lanzar hilo de escucha general (o usar server_escuchar que es bloqueante?)
    // server_escuchar es bloqueante. Deberiamos lanzarlo en un hilo aparte si queremos seguir haciendo cosas.
    // O si kernel main loop es solo esto...
    // Usualmente Kernel corre Planificacion + Consola + Server.
    // Lzaremos un thread para el server.
    
    pthread_t th;
    pthread_create(&th, NULL, (void*)server_listen_loop_io, NULL);
    pthread_detach(th);
}

void* server_listen_loop_io(void* arg) {
    server_escuchar(logger, "KERNEL_IO_SERVER", socket_server_io, handler_io_connection);
    return NULL;
}

static void* handler_io_connection(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    // Esperamos Handshake OP_HANDSHAKE_IO
    t_paquete* p = recibir_paquete(fd);
    if (!p) {
        close(fd);
        return NULL;
    }

    if (p->codigo_operacion == OP_HANDSHAKE_IO) {
        // Leer nombre
        char* nombre = paquete_read_string(p);
        log_info(logger, "Nueva interfaz conectada: %s (FD: %d)", nombre, fd);
        
        t_interfaz_io* io = malloc(sizeof(t_interfaz_io));
        io->nombre = nombre;
        io->socket = fd;
        list_add(lista_interfaces, io);

        send_ok(fd);
    } else {
        log_warning(logger, "Handshake invalido de IO");
    }
    paquete_destroy(p);

    // Loop de atencion a esta interfaz?
    // O la interfaz solo espera peticiones del Kernel?
    // Usualmente la interfaz se queda bloqueada esperando ordenes del Kernel,
    // salvo que quiera notificar fin de operacion.
    // SI la interfaz notifica fin de operacion, necesitamos un loop aqui recibiendo.
    // Asumamos que si.
    
    while(1) {
        t_paquete* msg = recibir_paquete(fd);
        if(!msg) {
             // Desconexion
             break;
        }
        // Manejar mensajes (e.g. FIN DE OPERACION IO)
        paquete_destroy(msg);
    }

    // Limpieza
    close(fd);
    return NULL;
}
