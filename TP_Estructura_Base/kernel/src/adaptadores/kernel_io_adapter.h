#ifndef KERNEL_IO_ADAPTER_H
#define KERNEL_IO_ADAPTER_H

#include <common/io/io_ops.h>
#include <kernel/pcb.h>

t_io_sleep* pcb_a_io_sleep(t_pcb* pcb, uint32_t tiempo);

#endif
