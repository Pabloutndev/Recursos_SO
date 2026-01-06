#include <server/server.h>

void server_escuchar(t_log* logger, char* nombre_server, int socket_server, void* (*handler)(void*)) {
    log_info(logger, "Servidor %s escuchando en socket %d", nombre_server, socket_server);

    while (1) {
        int cliente_fd = esperar_cliente(socket_server);
        if (cliente_fd < 0) {
            log_error(logger, "Error al aceptar cliente");
            continue;
        } 
        
        log_info(logger, "Cliente conectado (FD: %d)", cliente_fd);
        
        pthread_t hilo;
        int* fd_ptr = malloc(sizeof(int));
        *fd_ptr = cliente_fd;
        
        if (pthread_create(&hilo, NULL, handler, fd_ptr) != 0) {
            log_error(logger, "Error creando hilo para cliente");
            free(fd_ptr);
            close(cliente_fd);
            continue;
        }
        
        pthread_detach(hilo);
    }
}
