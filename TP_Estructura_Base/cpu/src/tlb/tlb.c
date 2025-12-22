#include "tlb.h"
#include <stdlib.h>
#include <string.h>
#include <commons/temporal.h>

static t_tlb tlb;
static uint64_t clock_lru = 0;

void tlb_init(uint32_t entradas, bool lru) {
    tlb.entradas = calloc(entradas, sizeof(t_tlb_entry));
    tlb.max = entradas;
    tlb.size = 0;
    tlb.lru = lru;
}

bool tlb_lookup(uint32_t pid, uint32_t pagina, uint32_t* marco) {
    for (uint32_t i = 0; i < tlb.size; i++) {
        if (tlb.entradas[i].pid == pid &&
            tlb.entradas[i].pagina == pagina) {

            tlb.entradas[i].last_use = ++clock_lru;
            *marco = tlb.entradas[i].marco;
            return true;
        }
    }
    return false;
}

static uint32_t elegir_victima() {
    if (!tlb.lru) return 0; // FIFO

    uint32_t min = 0;
    for (uint32_t i = 1; i < tlb.size; i++) {
        if (tlb.entradas[i].last_use < tlb.entradas[min].last_use)
            min = i;
    }
    return min;
}

void tlb_update(uint32_t pid, uint32_t pagina, uint32_t marco) {
    if (tlb.size < tlb.max) {
        tlb.entradas[tlb.size++] = (t_tlb_entry){
            .pid = pid,
            .pagina = pagina,
            .marco = marco,
            .last_use = ++clock_lru
        };
        return;
    }

    uint32_t victima = elegir_victima();
    tlb.entradas[victima] = (t_tlb_entry){
        .pid = pid,
        .pagina = pagina,
        .marco = marco,
        .last_use = ++clock_lru
    };
}

void tlb_clear_pid(uint32_t pid) {
    for (uint32_t i = 0; i < tlb.size; i++) {
        if (tlb.entradas[i].pid == pid) {
            memmove(&tlb.entradas[i], &tlb.entradas[i+1],
                    (tlb.size - i - 1) * sizeof(t_tlb_entry));
            tlb.size--;
            i--;
        }
    }
}
