#include <stdlib.h>
#include <common/cpu/contexto.h>
#include <common/cpu/tlb.h>
#include <serializacion/cpu.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

/// ==============================
/// CONTEXTO CPU
/// ==============================
/// ==============================
/// CONTEXTO CPU
/// ==============================
void enviar_contexto_cpu(int socket_dest, t_contexto_cpu* ctx)
{
    t_paquete* p = serializar_contexto_cpu(ctx);
    if (!p) return;

    enviar_paquete(socket_dest, p);
    eliminar_paquete(p);
}

t_contexto_cpu* recibir_contexto_cpu(t_paquete* p)
{
    return deserializar_contexto_cpu(p);
}

/// ==============================
/// PROCESS
/// ==============================
void enviar_proceso(int socket_dest, t_process* proc)
{
    t_paquete* p = serializar_process(proc);
    if (!p) return;

    enviar_paquete(socket_dest, p);
    eliminar_paquete(p);
}

t_process* recibir_proceso(t_paquete* p)
{
    return deserializar_process(p);
}

/// ==============================
/// TLB ENTRY
/// ==============================
void enviar_tlb_entry(int socket_dest, t_tlb_entry* entry)
{
    t_paquete* p = serializar_tlb_entry(entry);
    if (!p) return;

    enviar_paquete(socket_dest, p);
    eliminar_paquete(p);
}

t_tlb_entry* recibir_tlb_entry(t_paquete* p)
{
    return deserializar_tlb_entry(p);
}

/// ==============================
/// TLB COMPLETA
/// ==============================
void enviar_tlb(int socket_dest, t_tlb* tlb)
{
    t_paquete* p = serializar_tlb(tlb);
    if (!p) return;

    enviar_paquete(socket_dest, p);
    eliminar_paquete(p);
}

t_tlb* recibir_tlb(t_paquete* p)
{
    return deserializar_tlb(p);
}
