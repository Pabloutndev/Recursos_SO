#ifndef INTERRUPCIONES_H
#define INTERRUPCIONES_H

#include <stdint.h>

/**
 * @brief Desaloja un proceso que está en ejecución
 * @param pid ID del proceso a desalojar
 */
void desalojar_proceso(uint32_t pid);

/**
 * @brief Maneja una interrupción recibida desde CPU
 * @param pid ID del proceso que generó la interrupción
 * @param motivo Motivo de la interrupción (QUANTUM, IO, etc.)
 */
void manejar_interrupcion(uint32_t pid, const char* motivo);

#endif /* INTERRUPCIONES_H */
