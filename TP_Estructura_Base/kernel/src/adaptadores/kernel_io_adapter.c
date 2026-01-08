#include "kernel_io_adapter.h"
#include <stdlib.h>

t_io_sleep* pcb_a_io_sleep(t_pcb* pcb, uint32_t tiempo)
{
    t_io_sleep* io = malloc(sizeof(t_io_sleep));
    io->pid = pcb->pid;
    io->tiempo = tiempo;
    return io;
}
