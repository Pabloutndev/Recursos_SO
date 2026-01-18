#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include <model/model.h>
#include <paquete/paquete.h>

/// ===========
/// KERNEL Y CPU
/// ===========

/// ##### REQUESTS - SERIALIZACION y RESPONSES - DESERIALIZACION #####

t_paquete* serializar_contexto_cpu(t_contexto_cpu* ctx, op_code code);
t_contexto_cpu* deserializar_contexto_cpu(t_paquete* p);
/*
t_paquete* serializar_process(t_process* proc);
t_proceso* deserializar_process(t_paquete* p);
*/
/// ##### AUXILIAR SERIALIZACION/DESERIALIZACION DE REGISTROS

void serializar_registros(t_paquete* p, registros_t* r);
void deserializar_registros(t_paquete* p, registros_t* r);

/// ===========
/// IO
/// ===========

/// ##### REQUESTS - SERIALIZACION #####

t_paquete* serializar_io_sleep(const t_io_sleep* req);
t_io_sleep* deserializar_io_sleep(t_paquete* p);

t_paquete* serializar_io_fs_write(const t_io_fs_write* req);
t_io_fs_write* deserializar_io_fs_write(t_paquete* p);

t_paquete* serializar_io_fs_create(const t_io_fs_create* req);
t_io_fs_create* deserializar_io_fs_create(t_paquete* p);

/// ===========
/// MEMORIA
/// ===========
/// ##### REQUESTS - SERIALIZACION #####

t_paquete* serializar_mem_init_proceso(t_mem_init_proceso* req, op_code code);
t_mem_init_proceso* deserializar_mem_init_proceso(t_paquete* p);

t_paquete* serializar_mem_fin_proceso(t_mem_fin_proceso* req, op_code code);
t_mem_fin_proceso* deserializar_mem_fin_proceso(t_paquete* p);

t_paquete* serializar_mem_traducir_pagina(t_mem_traducir* req, op_code code);
t_mem_traducir* deserializar_mem_traducir_pagina(t_paquete* p);

t_paquete* serializar_mem_read(t_mem_read* req, op_code code);
t_mem_read* deserializar_mem_read(t_paquete* p);

t_paquete* serializar_mem_write(t_mem_write* req, op_code code);
t_mem_write* deserializar_mem_write(t_paquete* p);

t_paquete* serializar_mem_fetch(t_mem_fetch* req, op_code code);
t_mem_fetch* deserializar_mem_fetch(t_paquete* p);

/// ##### RESPONSES - DESERIALIZACION #####
t_paquete* serializar_mem_respuesta_traduccion(t_mem_respuesta_traduccion* res);
t_mem_respuesta_traduccion* deserializar_mem_respuesta_traduccion(t_paquete* p);

t_paquete* serializar_mem_respuesta_lectura(t_mem_respuesta_lectura* res);
t_mem_respuesta_lectura* deserializar_mem_respuesta_lectura(t_paquete* p);

t_paquete* serializar_mem_respuesta_instruccion(char* instruccion);
char* deserializar_mem_respuesta_instruccion(t_paquete* p);

#endif /* SERIALIZACION_H */
