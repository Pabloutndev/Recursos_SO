// dispatch.c
#include "dispatch.h"
#include <conexiones/cpu.h>
#include <commons/log.h>
#include <pcb/pcb.h>
#include <stdlib.h>

#include <mod_kernel.h>
// extern t_log* logger;

// Helper local para convertir PCB a Contexto
static void pcb_to_contexto(t_pcb* pcb, t_contexto_cpu* ctx) {
    ctx->pid = pcb->pid;
    ctx->pc = pcb->program_counter;
    ctx->registros = pcb->registros;
    // IO time field?
}

int enviar_proceso_a_cpu(t_pcb* pcb) {
    t_contexto_cpu ctx;
    pcb_to_contexto(pcb, &ctx);
    
    enviar_contexto_a_cpu(&ctx);
    return 1; // Success code (socket is valid globally)
}

int enviar_interrupt_cpu(uint32_t pid) {
    enviar_interrupcion_a_cpu(pid, 0); // Motivo 0 genérico
    return 1;
}

int recibir_contexto_actualizado(int sock, t_pcb* pcb) {
    return 0;
}
