#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/log.h>
#include <conexion/conexion.h>
#include <server/server.h>

extern t_log* logger;
extern t_log* loggerError;

// Prototipo de la funcion en io.c
void* handler_io_connection(void* arg);

// Prototipo de la funcion en conexiones/consola_handler.c
void* handler_consola_connection(void* arg);

static void* thread_server_io_runner(void* arg) {
    char* puerto = (char*)arg;
    int server_fd = iniciar_servidor(puerto);

    if (server_fd < 0) {
        log_error(loggerError, "Fallo iniciar server IO en puerto %s", puerto);
        free(puerto);
        return NULL;
    }

    log_info(logger, "Servidor KERNEL_IO escuchando en puerto %s", puerto);
    server_escuchar(logger, "KERNEL_IO", server_fd, handler_io_connection);

    close(server_fd);
    free(puerto);
    return NULL;
}

static void* thread_server_consola_runner(void* arg) {
    char* puerto = (char*)arg;
    int server_fd = iniciar_servidor(puerto);

    if (server_fd < 0) {
        log_error(loggerError, "Fallo iniciar server CONSOLA en puerto %s", puerto);
        free(puerto);
        return NULL;
    }

    log_info(logger, "Servidor KERNEL_CONSOLA escuchando en puerto %s", puerto);
    server_escuchar(logger, "KERNEL_CONSOLA", server_fd, handler_consola_connection);

    close(server_fd);
    free(puerto);
    return NULL;
}

void kernel_server_io_listen(char* puerto) {
    pthread_t th;
    char* p = strdup(puerto);
    pthread_create(&th, NULL, thread_server_io_runner, p);
    pthread_detach(th);
}

void kernel_server_consola_listen(char* puerto) {
    pthread_t th;
    char* p = strdup(puerto);
    pthread_create(&th, NULL, thread_server_consola_runner, p);
    pthread_detach(th);
}
