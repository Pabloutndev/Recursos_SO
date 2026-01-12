#ifndef DESERIALIZACION_MEMORIA_H
#define DESERIALIZACION_MEMORIA_H

#include <common/memoria/requests.h>
#include <common/memoria/responses.h>
#include <commons/collections/list.h>

/// REQUESTS
t_mem_init_proceso deserializar_mem_init_proceso(t_list* payload);
t_mem_fin_proceso deserializar_mem_fin_proceso(t_list* payload);

t_mem_traducir_pagina deserializar_mem_traducir_pagina(t_list* payload);
t_mem_rw deserializar_mem_rw(t_list* payload, void** data);

/// RESPONSE 
t_mem_respuesta_traduccion deserializar_mem_respuesta_traduccion(t_list* payload);
t_mem_respuesta_lectura deserializar_mem_respuesta_lectura(t_list* payload);

bool deserializar_mem_respuesta_escritura(t_list* payload);
bool deserializar_mem_respuesta_kernel(t_list* payload);

#endif
