#ifndef UTILS_PCK_H_
#define UTILS_PCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <pthread.h>
#include <semaphore.h>
#include <utils/ex_cpu.h>
#include <stddef.h>

/* CÓDIGOS DE OPERACIÓN (Protocolo) */
typedef enum
{ 
    // Genericos
    MENSAJE = 0,
    PAQUETE,
    OK = 10,
    FAIL = 11,

    // Conexiones / Handshakes
    HANDSHAKE_KERNEL = 100,
    HANDSHAKE_CPU = 101,
    HANDSHAKE_MEMORIA = 102,
    HANDSHAKE_IO = 103,

    // Kernel <-> CPU
    PROCESO_EXEC = 200,      // Kernel envía contexto a CPU para ejecutar
    INTERRUPCION_CPU = 201,  // Kernel envía interrupción
    FIN_DE_QUANTUM = 202,    // CPU devuelve contexto por fin de Q
    FIN_DE_PROCESO = 203,    // CPU devuelve por Exit
    BLOQUEO_IO = 204,        // CPU devuelve por IO
    WAIT_RECURSO = 205,      // CPU devuelve por Wait
    SIGNAL_RECURSO = 206,    // CPU devuelve por Signal
    ERROR_MEMORIA = 207,     // CPU devuelve por fallo de memoria incurable

    // Kernel/CPU <-> Memoria
    INIT_PROCESO = 300,      // Kernel -> Memoria: Crear estructuras
    FIN_PROCESO = 301,       // Kernel -> Memoria: Liberar estructuras
    ACCESO_TABLA = 302,      // CPU -> Memoria: Traducir Pagina a Frame
    LEER_MEMORIA = 303,      // CPU -> Memoria: Leer bytes
    ESCRIBIR_MEMORIA = 304,  // CPU -> Memoria: Escribir bytes
    FETCH_INSTRUCCION = 305, // CPU -> Memoria: Pedir instrucción (PC)
    AJUSTAR_TAMANIO = 306,   // Kernel -> Memoria: Resize

    // IO

    IO_STDIN = 400,
    IO_STDOUT = 401,
    IO_FS_CREATE = 402,
    IO_FS_DELETE = 403,
    IO_FS_TRUNCATE = 404,
    IO_FS_WRITE = 405,
    IO_FS_READ = 406,
    IO_GENERIC_SLEEP = 407,
    
    // ... completar segun requerimientos IO
    
} op_code;

/* Estructuras de Serialización */
typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

/* Primitivas de Paquetes */
t_paquete* crear_paquete(op_code codigo_op);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_entero_a_paquete(t_paquete* paquete, int x);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

/* Recepción */
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
t_list* recibir_paquete(int socket_cliente);

/* Serialización Auxiliar */
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente);
void enviar_codigo(int socket, int op_code);

/* Funciones especificas de Proceso (Legacy/Specific) */
// Se mantienen firmas pero sugiero usar paquetes genéricos
void enviar_contexto(int socket_client, void* ctx, int size_ctx); // Generic void* wrapper
// void recibir_contexto(...);

#endif /*UTILS_PCK_H_*/