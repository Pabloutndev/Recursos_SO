#include <protocolo/kernel.h>
#include <paquete/paquete.h>
#include <common/cpu/contexto.h>

/// ==============================
/// PROCESO (KERNEL-CPU)
/// ==============================
void enviar_proceso_exec(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = serializar_contexto_cpu(ctx, OP_PROCESO_EXEC);
    enviar_paquete(socket_dispatch, p);
    eliminar_paquete(p);
}

t_contexto_cpu* recibir_contexto_cpu(t_paquete* p)
{
    return deserializar_contexto_cpu(p);
}

/// ==============================
/// INTERRUPCION
/// ==============================
void enviar_interrupcion_cpu(int socket_interrupt)
{
    t_paquete* p = paquete_create(OP_INTERRUPCION_CPU);
    enviar_paquete(socket_interrupt, p);
    eliminar_paquete(p);
}

void enviar_fin_quantum(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = serializar_contexto_cpu(ctx, OP_FIN_DE_QUANTUM);
    enviar_paquete(p, socket_dispatch);
    eliminar_paquete(p);
}

void enviar_fin_proceso(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = serializar_contexto_cpu(ctx, OP_FIN_DE_PROCESO);
    enviar_paquete(p, socket_dispatch);
    eliminar_paquete(p);
}

void enviar_bloqueo_io(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = serializar_contexto_cpu(ctx, OP_BLOQUEO_IO);
    enviar_paquete(p, socket_dispatch);
    eliminar_paquete(p);
}
