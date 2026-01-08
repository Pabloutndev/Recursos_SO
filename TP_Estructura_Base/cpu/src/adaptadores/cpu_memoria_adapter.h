#ifndef CPU_MEMORIA_ADAPTER_H
#define CPU_MEMORIA_ADAPTER_H

#include <common/memoria/requests.h>
#include <stdint.h>

t_mem_fetch* cpu_a_mem_fetch(uint32_t pid, uint32_t pc);
t_mem_traducir_pagina* cpu_a_mem_traduccion(uint32_t pid, uint32_t pagina);

#endif
