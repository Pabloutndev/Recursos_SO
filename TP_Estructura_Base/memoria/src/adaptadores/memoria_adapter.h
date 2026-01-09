#ifndef MEMORIA_ADAPTER_H
#define MEMORIA_ADAPTER_H

#include <common/memoria/requests.h>

/* Kernel → Memoria */
void manejar_init_proceso(t_mem_init_proceso* req);
void manejar_fin_proceso(t_mem_fin_proceso* req);

/* CPU → Memoria */
void manejar_traduccion_pagina(t_mem_traducir_pagina* req, int socket_cpu);
void manejar_lectura_memoria(t_mem_rw* req, int socket_cpu);
void manejar_escritura_memoria(t_mem_rw* req, int socket_cpu);

#endif
