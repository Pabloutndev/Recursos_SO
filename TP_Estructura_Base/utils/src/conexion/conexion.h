#ifndef UTILS_CON_H_
#define UTILS_CON_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <pthread.h>
#include <paquete/paquete.h> // Include paquete for op_code if needed, or forward declare

/* Puertos Default */
#define PUERTO_KERNEL "8003"
#define PUERTO_MEMORIA "8002"
#define PUERTO_CPU "8006"
#define PUERTO_IO "8009"

/* Gestión de Sockets */
int iniciar_servidor(char* puerto);
int esperar_cliente(int socket_servidor);
int crear_conexion(char *ip, char* puerto);
void liberar_conexion(int socket_cliente);

/* Handshakes */
// Retorna true/false o checkea protocolo
bool handshake_cliente(int socket, int handshake_code, int handshake_expected, t_log* logger);

#endif /* UTILS_CON_H */
