#include <conexion/conexion.h>
#include <protocolo/op_code.h>
#include <paquete/paquete.h>
#include <commons/log.h>

int iniciar_servidor(char* puerto)
{
    int socket_servidor;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, puerto, &hints, &servinfo);

    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    
    // Setsockopt to reuse address (Important for fast restarts)
    int yes=1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if(bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Error binding socket");
        freeaddrinfo(servinfo);
        return -1;
    }

    listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
    return accept(socket_servidor, NULL, NULL);
}

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

int crear_conexion(char *ip, char* puerto)
{
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(ip, puerto, &hints, &server_info);

    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);

    return socket_cliente;
}

void liberar_conexion(int socket_cliente)
{
    close(socket_cliente);
}

bool handshake_cliente(int socket, int handshake_code, int handshake_expected, t_log* logger) {
    // Enviar OP_HANDSHAKE como paquete
    t_paquete* p = paquete_create(handshake_code);
    enviar_paquete(socket, p);
    paquete_destroy(p);

    // Recibir respuesta
    t_paquete* resp = recibir_paquete(socket);
    if (!resp) {
        if(logger) log_error(logger, "Error recibiendo respuesta de handshake");
        return false;
    }

    bool resultado = (resp->codigo_operacion == handshake_expected || resp->codigo_operacion == OP_OK);
    
    if (resultado) {
        if(logger) log_info(logger, "Handshake cliente exitoso");
    } else {
        if(logger) log_error(logger, "Handshake cliente fallido. Esperado: %d. Recibido: %d", handshake_expected, resp->codigo_operacion);
    }
    
    paquete_destroy(resp);
    return resultado;
}

// ============================================================================
// HANDSHAKE SERVIDOR
// ============================================================================
// Maneja el envío de respuesta OP_OK cuando recibe OP_HANDSHAKE.
// Uso: Llamar cuando se recibe OP_HANDSHAKE en el handler del servidor.
bool handshake_servidor(int socket, int handshake_response, t_log* logger) {
    // Enviar respuesta como paquete (típicamente OP_OK)
    t_paquete* p = paquete_create(handshake_response);
    bool resultado = enviar_paquete(socket, p);
    paquete_destroy(p);

    if (resultado) {
        if(logger) log_info(logger, "Handshake servidor respondido con OP_OK");
    } else {
        if(logger) log_error(logger, "Error al enviar respuesta de handshake");
    }

    return resultado;
}
