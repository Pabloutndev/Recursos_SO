
#ifndef KERNEL_CON_MEMORIA_H
#define KERNEL_CON_MEMORIA_H

#include <stdint.h>
#include <stdbool.h>

void conectar_memoria(char* ip, char* puerto);

// Solicita crear proceso en memoria
bool solicitar_creacion_proceso_memoria(uint32_t pid, int size);

// Solicita fin de proceso
void solicitar_fin_proceso_memoria(uint32_t pid);

#endif
