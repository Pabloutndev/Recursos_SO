#ifndef MEMORIA_ADAPTER_H
#define MEMORIA_ADAPTER_H

#include <common/memoria/requests.h>

void manejar_init_proceso(t_mem_init_proceso* req);
void manejar_fin_proceso(t_mem_fin_proceso* req);

#endif
