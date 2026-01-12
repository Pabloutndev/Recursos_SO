#ifndef PROTOCOLO_KERNEL_MEMORIA_H
#define PROTOCOLO_KERNEL_MEMORIA_H

#include <common/memoria/requests.h>

/* ========== REQUESTS (Kernel -> Memoria) ========== */

void enviar_init_proceso(int skt_memoria, t_mem_init_proceso* req);
void enviar_fin_proceso(int skt_memoria, t_mem_fin_proceso* req);

/* ========== RECEPCIÓN (Memoria -> Kernel) ========== */

bool recibir_respuesta_kernel(int skt_memoria);

#endif
