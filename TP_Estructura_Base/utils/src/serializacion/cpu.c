#include <serializacion/cpu.h>
#include <common/cpu/contexto.h>
#include <common/cpu/tlb.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

/// ##### REQUESTS - SERIALIZACION y RESPONSES - DESERIALIZACION #####

t_paquete* serializar_contexto_cpu(t_contexto_cpu* ctx, op_code code = OP_PROCESO_EXEC) {
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, ctx->pid);
    paquete_write_uint32(p, ctx->pc);
    paquete_write_uint32(p, ctx->quantum);
    paquete_write_uint32(p, ctx->finalizado);
    paquete_write_uint32(p, ctx->bloqueado);
    paquete_write_uint32(p, ctx->io_time);
    
    // Serializar registros
    serializar_registros(p, &ctx->registros);
    return p;
}

t_contexto_cpu* deserializar_contexto_cpu(t_paquete* p) {
    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));
    paquete_read_uint32(p, &ctx->pid);
    paquete_read_uint32(p, &ctx->pc);
    paquete_read_uint32(p, &ctx->quantum);
    paquete_read_uint32(p, &ctx->finalizado);
    paquete_read_uint32(p, &ctx->bloqueado);
    paquete_read_uint32(p, &ctx->io_time);

    deserializar_registros(p, &ctx->registros);
    return ctx;
}

t_paquete* serializar_process(t_process* proc) {
    t_paquete* p = paquete_create(OP_PROCESO_EXEC);
    paquete_write_int(p, proc->pid);
    paquete_write_int(p, proc->quantum);
    serializar_registros(p, &proc->registros);
    return p;
}

t_process* deserializar_process(t_paquete* p) {
    t_process* proc = malloc(sizeof(t_process));
    paquete_read_int(p, &proc->pid);
    paquete_read_int(p, &proc->quantum);
    deserializar_registros(p, &proc->registros);
    return proc;
}

t_paquete* serializar_tlb_entry(t_tlb_entry* entry) {
    t_paquete* p = paquete_create(OP_ACCESO_TABLA);
    paquete_write_int(p, entry->pid);
    paquete_write_int(p, entry->pagina);
    paquete_write_int(p, entry->marco);
    return entry;
}

t_tlb_entry* deserializar_tlb_entry(t_paquete* p) {
    t_tlb_entry* entry = malloc(sizeof(t_tlb_entry));
    paquete_read_int(p, &entry->pid);
    paquete_read_int(p, &entry->pagina);
    paquete_read_int(p, &entry->marco);
    return entry;
}

t_paquete* serializar_tlb(t_tlb* tlb) {
    t_paquete* p = paquete_create(OP_PAQUETE);
    paquete_write_int(p, tlb->size);
    paquete_write_int(p, tlb->max_entries);
    paquete_write_string(p, tlb->algorithm);

    for (int i = 0; i < tlb->size; i++) {
        serializar_tlb_entry(p, &tlb->entries[i]);
    }

    return p;
}

t_tlb* deserializar_tlb(t_paquete* p) {
    t_tlb* tlb = malloc(sizeof(t_tlb));
    paquete_read_int(p, &tlb->size);
    paquete_read_int(p, &tlb->max_entries);
    tlb->algorithm = paquete_read_string(p);

    tlb->entries = malloc(sizeof(t_tlb_entry) * tlb->size);
    for (int i = 0; i < tlb->size; i++) {
        deserializar_tlb_entry(p, &tlb->entries[i]);
    }
    return tlb;
}

t_paquete* serializar_fin_quatum(t_process* proc) {
    t_paquete* p = paquete_create(OP_FIN_DE_QUANTUM);
    paquete_write_int(p, proc->pid);
    paquete_write_int(p, proc->quantum);
    serializar_registros(p, &proc->registros);
    return p;
}

/// ##### AUXILIAR SERIALIZACION/DESERIALIZACION DE REGISTROS

void serializar_registros(t_paquete* p, registros_t* r) {
    // 8 bits
    paquete_write_uint8(p, r->AX);
    paquete_write_uint8(p, r->BX);
    paquete_write_uint8(p, r->CX);
    paquete_write_uint8(p, r->DX);

    // 32 bits
    paquete_write_uint32(p, r->EAX);
    paquete_write_uint32(p, r->EBX);
    paquete_write_uint32(p, r->ECX);
    paquete_write_uint32(p, r->EDX);
    paquete_write_uint32(p, r->SI);
    paquete_write_uint32(p, r->DI);
    paquete_write_uint32(p, r->PC);
}

void deserializar_registros(t_paquete* p, registros_t* r) {
    // 8 bits
    paquete_read_uint8(p, &r->AX);
    paquete_read_uint8(p, &r->BX);
    paquete_read_uint8(p, &r->CX);
    paquete_read_uint8(p, &r->DX);

    // 32 bits
    paquete_read_uint32(p, &r->EAX);
    paquete_read_uint32(p, &r->EBX);
    paquete_read_uint32(p, &r->ECX);
    paquete_read_uint32(p, &r->EDX);
    paquete_read_uint32(p, &r->SI);
    paquete_read_uint32(p, &r->DI);
    paquete_read_uint32(p, &r->PC);
}
