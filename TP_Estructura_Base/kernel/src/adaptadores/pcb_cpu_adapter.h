#ifndef KERNEL_PCB_CONTEXTO_H
#define KERNEL_PCB_CONTEXTO_H

#include <common/cpu/contexto.h>
#include <kernel/pcb.h>

t_contexto_cpu* pcb_a_contexto_cpu(t_pcb* pcb);
void contexto_cpu_a_pcb(t_contexto_cpu* ctx, t_pcb* pcb);

#endif
