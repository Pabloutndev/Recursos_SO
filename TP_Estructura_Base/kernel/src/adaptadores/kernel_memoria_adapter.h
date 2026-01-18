#ifndef KERNEL_MEMORIA_ADAPTER_H
#define KERNEL_MEMORIA_ADAPTER_H

#include <pcb/pcb.h>
#include <model/model.h>

/* ========================================
 * ADAPTADOR: Kernel → Memoria
 * 
 * Responsabilidad:
 * - Convertir PCB/datos internos a estructuras compartidas
 * - Enviar requests usando protocolo/mensajes
 * - Procesar respuestas de Memoria
 * ======================================== */

/* Transformación de estructuras */
t_mem_init_proceso* pcb_a_mem_init(t_pcb* pcb);
t_mem_fin_proceso* pcb_a_mem_fin(t_pcb* pcb);

/* OPERACIONES COMPLETAS (Request + Response) */

/**
 * kernel_init_proceso
 * 
 * Kernel solicita a Memoria crear un proceso.
 * Espera respuesta bloqueante.
 * 
 * @param pcb: Process Control Block con PID y tamaño
 * @return true si Memoria confirmó, false si rechazó
 */
bool kernel_init_proceso(t_pcb* pcb);

/**
 * kernel_fin_proceso
 * 
 * Kernel notifica a Memoria que un proceso finaliza.
 * No espera respuesta (one-way).
 * 
 * @param pid: Identificador del proceso
 */
void kernel_fin_proceso(uint32_t pid);

#endif