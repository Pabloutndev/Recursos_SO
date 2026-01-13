#ifndef SERIALIZACION_MEMORIA_H
#define SERIALIZACION_MEMORIA_H

#include <paquete/paquete.h>
#include <common/memoria/memoria.h>

/// ##### REQUESTS - SERIALIZACION #####

t_paquete* serializar_mem_init_proceso(t_mem_init_proceso* req);
t_mem_init_proceso* deserializar_mem_init_proceso(t_paquete* p);

t_paquete* serializar_mem_fin_proceso(t_mem_fin_proceso* req);
t_mem_fin_proceso* deserializar_mem_fin_proceso(t_paquete* p);

t_paquete* serializar_mem_traducir_pagina(t_mem_traducir_pagina* req);
t_mem_traducir_pagina* deserializar_mem_traducir_pagina(t_paquete* p);

t_paquete* serializar_mem_read(t_mem_read* req);
t_mem_read* deserializar_mem_read(t_paquete* p);

t_paquete* serializar_mem_write(t_mem_write* req);
t_mem_write* deserializar_mem_write(t_paquete* p);

t_paquete* serializar_mem_fetch(t_mem_fetch* req);
t_mem_fetch* deserializar_mem_fetch(t_paquete* p);

/// ##### RESPONSES - DESERIALIZACION #####

t_paquete* serializar_mem_respuesta_traduccion(t_mem_respuesta_traduccion* res);
t_mem_respuesta_traduccion* deserializar_mem_respuesta_traduccion(t_paquete* p);

t_paquete* serializar_mem_respuesta_lectura(t_mem_respuesta_lectura* res);
t_mem_respuesta_lectura* deserializar_mem_respuesta_lectura(t_paquete* p);

t_paquete* serializar_mem_respuesta_instruccion(char* instruccion);
char* deserializar_mem_respuesta_instruccion(t_paquete* p);

#endif
