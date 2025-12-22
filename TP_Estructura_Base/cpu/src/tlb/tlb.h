#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t pid;
    uint32_t pagina;
    uint32_t marco;
    uint64_t last_use;   // para LRU
} t_tlb_entry;

typedef struct {
    t_tlb_entry* entradas;
    uint32_t size;
    uint32_t max;
    bool lru;
} t_tlb;

void tlb_init(uint32_t entradas, bool lru);
bool tlb_lookup(uint32_t pid, uint32_t pagina, uint32_t* marco);
void tlb_update(uint32_t pid, uint32_t pagina, uint32_t marco);
void tlb_clear_pid(uint32_t pid);

#endif
