#ifndef CPU_KERNEL_H
#define CPU_KERNEL_H

#include <stdbool.h>
#include <contexto/contexto.h>

void conexiones_kernel_init(char* ip, char* puerto);
bool kernel_recibir_contexto(contexto_t* ctx);
void kernel_enviar_contexto(const contexto_t* ctx);
void kernel_enviar_interrupcion(int tipo);

#endif
