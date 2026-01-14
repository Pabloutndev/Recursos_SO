#include <common/memoria/memoria.h>
#include <stdlib.h>
#include <stdint.h>

t_mem_fetch* cpu_a_mem_fetch(uint32_t pid, uint32_t pc)
{
    t_mem_fetch* req = malloc(sizeof(t_mem_fetch));
    req->pid = pid;
    req->pc = pc;
    return req;
}

t_mem_traducir* cpu_a_mem_traduccion(uint32_t pid, uint32_t pagina)
{
    t_mem_traducir* req = malloc(sizeof(t_mem_traducir));
    req->pid = pid;
    req->direccion_logica = pagina;
    return req;
}