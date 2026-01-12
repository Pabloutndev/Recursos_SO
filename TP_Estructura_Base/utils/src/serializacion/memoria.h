#ifndef SERIALIZACION_MEMORIA_H
#define SERIALIZACION_MEMORIA_H

#include <paquete/paquete.h>
#include <common/memoria/requests.h>
#include <common/memoria/responses.h>

/* Requests */
/* Kernel -> Memoria */
t_paquete* serializar_mem_init_proceso(t_mem_init_proceso* req);
t_paquete* serializar_mem_fin_proceso(t_mem_fin_proceso* req);

/* CPU -> Memoria */
t_paquete* serializar_mem_traducir_pagina(t_mem_traducir_pagina* req);
t_paquete* serializar_mem_rw(op_code code, t_mem_rw* req, void* data);

/* Responses */
t_paquete* serializar_mem_respuesta_traduccion(t_mem_respuesta_traduccion* resp);
t_paquete* serializar_mem_respuesta_lectura(t_mem_respuesta_lectura* resp);
t_paquete* serializar_mem_respuesta_escritura(t_mem_respuesta_lectura* resp);
t_paquete* serializar_mem_respuesta_kernel(bool ok);

#endif
