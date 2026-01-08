#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stdbool.h>
#include <contexto/contexto.h>

void mmu_set_contexto(const t_contexto_cpu* ctx);
uint32_t mmu_traducir(uint32_t dir_logica, bool escritura);

#endif
