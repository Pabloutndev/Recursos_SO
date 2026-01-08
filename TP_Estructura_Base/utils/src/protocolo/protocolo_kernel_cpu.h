#ifndef PROTOCOLO_KERNEL_CPU_H
#define PROTOCOLO_KERNEL_CPU_H

#include <common/cpu/contexto.h>

// Kernel -> CPU
void enviar_proceso_exec(int socket_dispatch, t_contexto_cpu* ctx);
void enviar_interrupcion_cpu(int socket_interrupt);

// CPU -> Kernel
void enviar_fin_quantum(int socket_dispatch, t_contexto_cpu* ctx);
void enviar_fin_proceso(int socket_dispatch, t_contexto_cpu* ctx);
void enviar_bloqueo_io(int socket_dispatch, t_contexto_cpu* ctx);

t_contexto_cpu* recibir_contexto_cpu(int socket_dispatch);

#endif
