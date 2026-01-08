#include "kernel_memoria_adapter.h"
#include <stdlib.h>

t_mem_init_proceso* pcb_a_mem_init(t_pcb* pcb)
{
    t_mem_init_proceso* req = malloc(sizeof(t_mem_init_proceso));
    req->pid = pcb->pid;
    req->tamanio = pcb->tam_proceso;
    return req;
}

t_mem_fin_proceso* pcb_a_mem_fin(t_pcb* pcb)
{
    t_mem_fin_proceso* req = malloc(sizeof(t_mem_fin_proceso));
    req->pid = pcb->pid;
    return req;
}
