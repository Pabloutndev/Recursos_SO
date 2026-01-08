#include "pcb_contexto.h"
#include <stdlib.h>

t_contexto_cpu* pcb_a_contexto_cpu(t_pcb* pcb)
{
    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));

    ctx->pid = pcb->pid;
    ctx->pc = pcb->program_counter;
    ctx->quantum = pcb->quantum;
    ctx->registros = pcb->registros;

    return ctx;
}

void contexto_cpu_a_pcb(t_contexto_cpu* ctx, t_pcb* pcb)
{
    pcb->program_counter = ctx->pc;
    pcb->registros = ctx->registros;
}
