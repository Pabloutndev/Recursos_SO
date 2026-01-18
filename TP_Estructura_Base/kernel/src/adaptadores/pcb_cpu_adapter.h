#ifndef KERNEL_PCB_CONTEXTO_H
#define KERNEL_PCB_CONTEXTO_H

#include <model/model.h>
#include <pcb/pcb.h>

/* ========================================
 * ADAPTADOR: Kernel ↔ CPU
 * 
 * Responsabilidad:
 * - Convertir entre PCB y Contexto CPU
 * - PCB es estructura interna de Kernel
 * - Contexto CPU es estructura compartida
 * ======================================== */

/**
 * pcb_a_contexto_cpu
 * Convierte un PCB de Kernel a Contexto CPU para enviar a CPU.
 * Extrae: PID, PC, Quantum, Registros
 * 
 * @return Estructura dinámica (debe liberarse)
 */
t_contexto_cpu* pcb_a_contexto_cpu(t_pcb* pcb);

/**
 * contexto_cpu_a_pcb
 * Actualiza un PCB con datos del Contexto CPU recibido.
 * Actualiza: PC, Registros
 * Nota: NO modifica PID, Quantum (controlados por Kernel)
 */
void contexto_cpu_a_pcb(t_contexto_cpu* ctx, t_pcb* pcb);

/**
 * kernel_cpu_adapter_dispatch
 * OPERACIÓN COMPLETA: Kernel envía PCB a CPU para ejecutar.
 * 
 * Responsabilidades:
 * 1. Convertir PCB → Contexto CPU
 * 2. Enviar a CPU usando protocolo
 * 3. CPU ejecutará y devolverá contexto actualizado
 * 
 * Nota: Esta función es bloqueante. CPU ejecuta y retorna contexto.
 * El manejador de CPU Dispatch debe procesar la respuesta.
 */
void kernel_dispatch(t_pcb* pcb);

/**
 * kernel_interrupt
 * Envía interrupción a CPU para desalojar proceso actual.
 * 
 * Usos:
 * - Fin de Quantum
 * - Evento IO
 * - Desalojo por razones externas
 */
void kernel_interrupt(void);

#endif
