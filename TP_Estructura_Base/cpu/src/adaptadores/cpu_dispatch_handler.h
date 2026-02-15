#ifndef CPU_DISPATCH_HANDLER_H
#define CPU_DISPATCH_HANDLER_H

#include <paquete/paquete.h>

/**
 * cpu_handler_atender_ejecucion
 *
 * Handler principal del socket dispatch.
 * Recibe contexto, ejecuta ciclo de instruccion,
 * determina el opcode de respuesta y envia resultado a Kernel.
 */
void cpu_handler_atender_ejecucion(int fd, t_paquete* p);

/**
 * cpu_handler_atender_interrupcion
 *
 * Handler del socket interrupt.
 * Registra la interrupcion recibida desde Kernel.
 */
void cpu_handler_atender_interrupcion(int fd, t_paquete* p);

#endif
