#ifndef PROTOCOLO_CPU_MEMORIA_H
#define PROTOCOLO_CPU_MEMORIA_H

#include <common/memoria/requests.h>

// CPU -> Memoria
void enviar_fetch_instruccion(int socket_memoria, t_mem_fetch* req);
void enviar_traduccion_pagina(int socket_memoria, t_mem_traducir_pagina* req);
void enviar_lectura_memoria(int socket_memoria, t_mem_rw* req);
void enviar_escritura_memoria(int socket_memoria, t_mem_rw* req);

// Memoria -> CPU
char* recibir_instruccion(int socket_memoria);
uint32_t recibir_frame(int socket_memoria);

#endif
