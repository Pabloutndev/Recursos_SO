#ifndef SERIALIZACION_CPU_H_
#define SERIALIZACION_CPU_H_

#include <common/cpu/contexto.h>
#include <common/cpu/tlb.h>
#include <paquete/paquete.h>

/// ##### REQUESTS - SERIALIZACION y RESPONSES - DESERIALIZACION #####

t_paquete* serializar_contexto_cpu(t_contexto_cpu* ctx);
t_contexto_cpu* deserializar_contexto_cpu(t_paquete* p);

t_paquete* serializar_process(t_process* proc);
t_process* deserializar_process(t_paquete* p);

t_paquete* serializar_tlb_entry(t_tlb_entry* entry);
t_tlb_entry* deserializar_tlb_entry(t_paquete* p);

t_paquete* serializar_tlb(t_tlb* tlb);
t_tlb* deserializar_tlb(t_paquete* p);

/// ##### AUXILIAR SERIALIZACION/DESERIALIZACION DE REGISTROS

void serializar_registros(t_paquete* p, registros_t* r);
void deserializar_registros(t_paquete* p, registros_t* r);

#endif /* SERIALIZACION_CPU_H_ */