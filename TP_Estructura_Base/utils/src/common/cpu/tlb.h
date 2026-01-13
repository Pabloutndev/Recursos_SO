#ifndef CPU_TLB_H
#define CPU_TLB_H

#include <stdint.h>

// Estructura para representar una entrada de la TLB
typedef struct {
    int pid;
    int pagina;
    int marco;
} t_tlb_entry;

// Estructura para representar la TLB
typedef struct {
    t_tlb_entry *entries;
    int size;
    int max_entries;
    char *algorithm; // FIFO o LRU
} t_tlb;

#endif /* CPU_TLB_H */