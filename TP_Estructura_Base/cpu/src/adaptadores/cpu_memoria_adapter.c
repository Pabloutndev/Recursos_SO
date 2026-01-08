#include "cpu_memoria_adapter.h"
#include <stdlib.h>

t_mem_fetch* cpu_a_mem_fetch(uint32_t pid, uint32_t pc)
{
    t_mem_fetch* req = malloc(sizeof(t_mem_fetch));
    req->pid = pid;
    req->pc = pc;
    return req;
}

t_mem_traducir_pagina* cpu_a_mem_traduccion(uint32_t pid, uint32_t pagina)
{
    t_mem_traducir_pagina* req = malloc(sizeof(t_mem_traducir_pagina));
    req->pid = pid;
    req->pagina = pagina;
    return req;
}
