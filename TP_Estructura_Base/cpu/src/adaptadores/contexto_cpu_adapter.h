#ifndef CPU_CONTEXTO_ADAPTER_H
#define CPU_CONTEXTO_ADAPTER_H

#include <common/cpu/contexto.h>

void cargar_contexto_cpu(t_contexto_cpu* ctx);
t_contexto_cpu* extraer_contexto_cpu(void);

#endif
