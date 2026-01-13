#include <stdlib.h>
#include <common/cpu/contexto.h>
#include <common/cpu/tlb.h>
#include <serializacion/cpu.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

/// ==============================
/// CONTEXTO CPU
/// ==============================
void enviar_contexto_cpu(int socket_dest, t_contexto_cpu* ctx);
t_contexto_cpu* recibir_contexto_cpu(t_paquete* p);

/// ==============================
/// PROCESS
/// ==============================
void enviar_proceso(int socket_dest, t_process* proc);
t_process* recibir_proceso(t_paquete* p);

/// ==============================
/// TLB ENTRY
/// ==============================
void enviar_tlb_entry(int socket_dest, t_tlb_entry* entry);
t_tlb_entry* recibir_tlb_entry(t_paquete* p);

/// ==============================
/// TLB COMPLETA
/// ==============================
void enviar_tlb(int socket_dest, t_tlb* tlb);
t_tlb* recibir_tlb(t_paquete* p);
