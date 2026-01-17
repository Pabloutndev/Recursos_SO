#ifndef UTILS_PCK_H_
#define UTILS_PCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stddef.h>

#include <pthread.h>
#include <semaphore.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <model/model.h>

#include <protocolo/op_code.h>

/* Estructuras de Serialización */
typedef struct
{
	uint32_t size;
    uint32_t offset;
	void* stream;
} t_buffer;

typedef struct 
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

///NOTE: API FOR SEND/RECV PACKAGE BY SOCKETS
t_paquete* paquete_create(op_code code);
t_buffer* buffer_create(void);
void paquete_destroy(t_paquete* p);

/// NOTE: SEND / RECV
bool enviar_paquete(int fd, t_paquete* p);
t_paquete* recibir_paquete(int fd);
op_code recibir_operacion(t_paquete* p);
op_code devolver_operacion(t_paquete* p);

/// NOTE: SERIALIZACION / DESERIALIZACION
bool paquete_write_buffer(t_paquete*, const void*, uint32_t);
void* paquete_read_buffer(t_paquete*, uint32_t*);

bool paquete_write_string(t_paquete* p, const char* s);
char* paquete_read_string(t_paquete* p);

/// NOTE: TIPOS DE DATOS (SERIALIZACION / DESERIALIZACION)
bool paquete_write_uint8(t_paquete*, uint8_t);
bool paquete_read_uint8(t_paquete*, uint8_t*);

bool paquete_write_uint32(t_paquete*, uint32_t);
bool paquete_read_uint32(t_paquete*, uint32_t*);

bool paquete_write_int(t_paquete*, int);
bool paquete_read_int(t_paquete*, int*);

bool paquete_write_float(t_paquete*, float);
bool paquete_read_float(t_paquete*, float*);

bool paquete_write_bool(t_paquete*, bool);
bool paquete_read_bool(t_paquete*, bool*);

#endif /*UTILS_PCK_H_*/