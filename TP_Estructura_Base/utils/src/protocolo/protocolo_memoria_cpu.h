#ifndef PROTOCOLO_MEMORIA_CPU_H
#define PROTOCOLO_MEMORIA_CPU_H

#include <common/memoria/requests.h>
#include <common/memoria/responses.h>

void enviar_traduccion_pagina(int skt, t_mem_traducir_pagina* req);
void enviar_respuesta_traduccion(int skt, t_mem_respuesta_traduccion* resp);

void enviar_lectura_memoria(int skt, t_mem_rw* req);
void enviar_respuesta_lectura(int skt, t_mem_respuesta_lectura* resp);

void enviar_escritura_memoria(int skt, t_mem_rw* req);
void enviar_respuesta_escritura(int skt, bool ok);

/* AUX */
t_mem_traducir_pagina recibir_traduccion_pagina(int skt);

#endif