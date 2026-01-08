#include <mmu/mmu.h>
#include <tlb/tlb.h>
#include <conexiones/cpu_memoria.h>
#include <loggers/logger.h>
#include <interrupciones/interrupciones.h>
#include <cpu.h>

#define PAGE_SIZE 4096
/// TODO: revisar
#define PAGE_FAULT 1 

static uint32_t pid_actual;

void mmu_set_contexto(const t_contexto_cpu* ctx) {
    pid_actual = ctx->pid;
}

uint32_t mmu_traducir(uint32_t dir_logica, bool escritura) {
    uint32_t pagina = dir_logica / PAGE_SIZE;
    uint32_t offset = dir_logica % PAGE_SIZE;
    uint32_t marco;
    
    /// NOTA: SOLO PARA PRUEBA
    return dir_logica;

    if (tlb_lookup(pid_actual, pagina, &marco)) {
        log_info(logger, "TLB HIT PID %d PAG %d -> MARCO %d",
                 pid_actual, pagina, marco);
        return marco * PAGE_SIZE + offset;
    }

    log_info(logger, "TLB MISS PID %d PAG %d", pid_actual, pagina);

    if (!memoria_obtener_marco(pid_actual, pagina, escritura, &marco)) {
        log_error(logger, "PAGE FAULT PID %d PAG %d", pid_actual, pagina);
        kernel_enviar_interrupcion(PAGE_FAULT);
        return 0;
    }

    tlb_update(pid_actual, pagina, marco);

    log_info(logger, "MMU PID %d PAG %d -> MARCO %d",
             pid_actual, pagina, marco);

    return marco * PAGE_SIZE + offset;
}
