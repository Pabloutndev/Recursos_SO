#ifndef KERNEL_MEMORIA_ADAPTER_H
#define KERNEL_MEMORIA_ADAPTER_H

#include <pcb/pcb.h>
#include <common/memoria/requests.h>

t_mem_init_proceso* pcb_a_mem_init(t_pcb* pcb);
t_mem_fin_proceso* pcb_a_mem_fin(t_pcb* pcb);

#endif