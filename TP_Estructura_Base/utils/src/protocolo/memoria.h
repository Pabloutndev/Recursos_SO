#ifndef PROTOCOLO_MEMORIA_H_
#define PROTOCOLO_MEMORIA_H_

#include <common/cpu/cpu.h>
#include <common/memoria/memoria.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <serializacion/memoria.h>
#include <protocolo/memoria.h>

/// ==============================
/// FETCH INSTRUCCION
/// ==============================
void enviar_fetch_instruccion(int socket_memoria, t_mem_fetch* req);
t_mem_fetch* recibir_fetch(t_paquete* p);

/// ==============================
/// PROCESS
/// ==============================
void enviar_init_proceso(int socket_memoria, t_mem_init_proceso* req);
void enviar_fin_proceso(int socket_memoria, t_mem_fin_proceso* req);
bool recibir_respuesta_kernel(int socket_memoria);

/// ==============================
/// TRADUCIR PAGINA
/// ==============================
void enviar_traduccion_pagina(int socket_memoria, t_mem_traducir_pagina* req);
t_mem_traducir_pagina* recibir_mem_traducir_pagina(t_paquete* p);

/// ==============================
/// LEER/ESCRIBIR MEMORIA
/// ==============================
void enviar_lectura_memoria(int socket_memoria, t_mem_read* req);
void enviar_escritura_memoria(int socket_memoria, t_mem_write* req);
t_mem_read* recibir_lectura_memoria(t_paquete* p);
t_mem_write* recibir_escritura_memoria(t_paquete* p);

/// ==============================
/// RESPUESTA LECTURA
/// ==============================
void enviar_respuesta_lectura(int socket_memoria, t_mem_respuesta_lectura* req);
t_mem_respuesta_lectura* recibir_instruccion(t_paquete* p);

/// ==============================
/// RESPUESTA TRADUCCION
/// ==============================
void enviar_respuesta_traduccion(int socket_memoria, t_mem_respuesta_traduccion* req);
t_mem_respuesta_traduccion* recibir_respuesta_traduccion(t_paquete* p);

/// ==============================
/// RESPUESTA INSTRUCCION 
/// ==============================
void enviar_respuesta_instruccion(int socket_memoria, char* instruccion);
char* recibir_respuesta_instruccion(t_paquete* p);

#endif /* PROTOCOLO_MEMORIA_H_ */