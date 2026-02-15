#ifndef CPU_HANDLERS_H
#define CPU_HANDLERS_H

#include <model/model.h>

/**
 * @brief Maneja un SEGFAULT reportado por CPU.
 * Actualiza el contexto del PCB y finaliza el proceso.
 */
void cpu_handler_segfault(t_contexto_cpu* ctx);

/**
 * @brief Maneja una solicitud IO_STDIN_READ desde CPU.
 * Busca la interfaz STDIN, envia la solicitud y bloquea el proceso.
 */
void cpu_handler_io_stdin_read(t_contexto_cpu* ctx);

/**
 * @brief Maneja una solicitud IO_STDOUT_WRITE desde CPU.
 * Busca la interfaz STDOUT, envia la solicitud y bloquea el proceso.
 */
void cpu_handler_io_stdout_write(t_contexto_cpu* ctx);

#endif /* CPU_HANDLERS_H */
