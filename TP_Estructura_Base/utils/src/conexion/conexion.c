#include "conexion.h"

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
    // Envio mi handshake
    int hs = handshake_code;
    send(socket, &hs, sizeof(int), 0);

    // Recibo respuesta
    int response;
    recv(socket, &response, sizeof(int), MSG_WAITALL);

    if (response == handshake_expected || response == OK) {
        // Logica flexible: a veces validamos que nos devuelvan OK o el mismo handshake
        if(logger) log_info(logger, "Handshake exitoso");
        return true;
    } else {
        if(logger) log_error(logger, "Handshake fallido. Esperado: %d. Recibido: %d", handshake_expected, response);
        return false;
    }
}
