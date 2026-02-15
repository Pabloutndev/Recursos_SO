#include <mmu/mmu.h>
#include <tlb/tlb.h>
#include <conexiones/cpu_memoria.h>
#include <loggers/logger.h>
#include <interrupciones/interrupciones.h>
#include <cpu.h>

static uint32_t pid_actual = UINT32_MAX;

void mmu_set_contexto(const t_contexto_cpu* ctx) {
    if (pid_actual != ctx->pid) {
        log_info(CPU_CTX.logger, "MMU: Cambio de PID (%u -> %u). Limpiando TLB del PID anterior.", pid_actual, ctx->pid);
        tlb_clear_pid(pid_actual);
        pid_actual = ctx->pid;
    }
}

uint32_t mmu_traducir(uint32_t dir_logica, bool escritura) {
    uint32_t tam_pag = CPU_CTX.config.tam_pagina;
    uint32_t pagina = dir_logica / tam_pag;
    uint32_t offset = dir_logica % tam_pag;
    uint32_t marco;
    
    // Check TLB
    if (tlb_lookup(pid_actual, pagina, &marco)) {
        log_info(CPU_CTX.logger, "TLB HIT PID %u PAG %d -> MARCO %d",
                 pid_actual, pagina, marco);
        return marco * tam_pag + offset;
    }

    log_info(CPU_CTX.logger, "TLB MISS PID %u PAG %d", pid_actual, pagina);

    // Request to Memory
    if (!memoria_obtener_marco(pid_actual, pagina, escritura, &marco)) {
        log_error(CPU_CTX.logger, "SEGFAULT PID %u PAG %d", pid_actual, pagina);
        CPU_CTX.motivo_desalojo = MOTIVO_SEGFAULT;
        return TRADUCCION_ERROR;
    }

    // Update TLB
    tlb_update(pid_actual, pagina, marco);

    log_info(CPU_CTX.logger, "MMU PID %u PAG %d -> MARCO %d",
             pid_actual, pagina, marco);

    return marco * tam_pag + offset;
}
