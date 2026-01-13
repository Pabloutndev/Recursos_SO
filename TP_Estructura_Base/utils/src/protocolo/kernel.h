#ifndef PROTOCOLO_KERNEL_H_
#define PROTOCOLO_KERNEL_H_

#include <paquete/paquete.h>
#include <common/cpu/contexto.h>

/// ==============================
/// PROCESO (KERNEL-CPU)
/// ==============================
void enviar_proceso_exec(int socket_dispatch, t_contexto_cpu* ctx);
t_contexto_cpu* recibir_contexto_cpu(t_paquete* p);

/// ==============================
/// INTERRUPCION
/// ==============================
void enviar_interrupcion_cpu(int socket_interrupt);
void enviar_fin_quantum(int socket_dispatch, t_contexto_cpu* ctx);
void enviar_fin_proceso(int socket_dispatch, t_contexto_cpu* ctx);
void enviar_bloqueo_io(int socket_dispatch, t_contexto_cpu* ctx);

#endif /* PROTOCOLO_KERNEL_H_ */