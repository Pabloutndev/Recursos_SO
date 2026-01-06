
#ifndef UTILS_SERVER_H_
#define UTILS_SERVER_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <commons/log.h>
#include <conexion/conexion.h>

// Inicia servidor en puerto y se queda escuchando conexiones en un loop infinito.
// Por cada conexión lanza un hilo con la función handler.
// El handler recibe un int* con el file descriptor del cliente.
void server_escuchar(t_log* logger, char* nombre_server, int socket_server, void* (*handler)(void*));

#endif
