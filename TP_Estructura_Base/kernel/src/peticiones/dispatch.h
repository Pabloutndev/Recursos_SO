#ifndef DISPATCH_H
#define DISPATCH_H

#include <pcb/pcb.h>
#include <stdint.h>

/**
 * @brief Envía un proceso a CPU para ejecución
 * @param pcb Proceso a enviar
 * @return int Socket de conexión o -1 en caso de error
 */
int enviar_proceso_a_cpu(t_pcb* pcb);

/**
 * @brief Envía una interrupción a CPU para desalojar un proceso
 * @param pid ID del proceso a desalojar
 * @return int 0 si éxito, -1 si error
 */
int enviar_interrupt_cpu(uint32_t pid);

/**
 * @brief Recibe el contexto actualizado desde CPU
 * @param sock Socket de conexión
 * @param pcb PCB a actualizar
 * @return int 0 si éxito, -1 si error
 */
int recibir_contexto_actualizado(int sock, t_pcb* pcb);

#endif /* DISPATCH_H */