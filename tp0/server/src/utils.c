#include"utils.h"

int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, PUERTO, &hints, &servinfo);

    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_servidor == -1) {
        log_error(logger, "Error creando socket");
        freeaddrinfo(servinfo);
        return -1;
    }

    int opt = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) != 0) {
        log_error(logger, "Error en bind()");
        freeaddrinfo(servinfo);
        close(socket_servidor);
        return -1;
    }

	if (listen(socket_servidor, SOMAXCONN) != 0) {
        log_error(logger, "Error en listen()");
        close(socket_servidor);
        return -1;
    }

	freeaddrinfo(servinfo);

	log_info(logger, "Servidor listo. Esperando conexiones...");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	int socket_cliente = accept(socket_servidor, NULL, NULL);
    if (socket_cliente == -1) {
        log_error(logger, "Error en accept()");
        return -1;
    }
	log_info(logger, "Se conecto un cliente!");
	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	int ret = recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	if (ret <= 0) return NULL;

	void* buffer = malloc(*size);

	ret = recv(socket_cliente, buffer, *size, MSG_WAITALL);
	if (ret <= 0) {
		free(buffer);
		return NULL;
	}

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	if (!buffer) {
        log_warning(logger, "Cliente desconectado mientras recibía mensaje");
        return;
    }
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}
