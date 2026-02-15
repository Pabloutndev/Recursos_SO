#ifndef CPU_CONTEXTO_ADAPTER_H
#define CPU_CONTEXTO_ADAPTER_H

#include <model/model.h>
#include <paquete/paquete.h>

/* ========================================
 * ADAPTADOR: CPU - Contexto Local
 *
 * Responsabilidad:
 * - Cargar contexto recibido de Kernel en CPU
 * - Extraer contexto después de ejecución
 * - Mantener estado CPU sincronizado
 * ======================================== */

bool recibir_contexto_kernel(t_contexto_cpu* ctx);
void enviar_contexto_kernel(t_contexto_cpu* ctx, t_motivo_desalojo motivo);

/**
 * cpu_contexto_adapter_cargar
 *
 * Carga un contexto recibido de Kernel en el estado local de CPU.
 * CPU usará estos valores durante ejecución.
 *
 * @param ctx: Contexto compartido recibido de Kernel
 */
void cpu_contexto_adapter_cargar(t_contexto_cpu* ctx);

/**
 * cpu_contexto_adapter_extraer
 *
 * Extrae contexto actual de CPU para enviar de vuelta a Kernel.
 * Se llama después de ejecutar ciclo de instrucción.
 *
 * @return Contexto actualizado (debe liberarse)
 */
t_contexto_cpu* cpu_contexto_adapter_extraer(void);

#endif
