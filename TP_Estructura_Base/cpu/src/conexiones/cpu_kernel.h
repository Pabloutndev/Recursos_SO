
#ifndef CPU_KERNEL_H
#define CPU_KERNEL_H

#include <stdbool.h>
#include <contexto/contexto.h>

// Inicia los servicios de servidor para Kernel (Dispatch e Interrupt)
// Esta funcion lanza hilos y no bloquea al caller.
void cpu_servidores_kernel_init(char* puerto_dispatch, char* puerto_interrupt);

#endif
