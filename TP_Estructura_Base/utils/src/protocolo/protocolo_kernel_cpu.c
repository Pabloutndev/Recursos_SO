#include "protocolo_kernel_cpu.h"
#include <paquete/paquete.h>

void enviar_proceso_exec(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = crear_paquete(PROCESO_EXEC);

    agregar_entero_a_paquete(p, ctx->pid);
    agregar_entero_a_paquete(p, ctx->pc);
    agregar_entero_a_paquete(p, ctx->quantum);
    agregar_a_paquete(p, &ctx->registros, sizeof(registros_t));

    enviar_paquete(p, socket_dispatch);
    eliminar_paquete(p);
}

void enviar_interrupcion_cpu(int socket_interrupt)
{
    enviar_codigo(socket_interrupt, INTERRUPCION_CPU);
}

// ================= CPU -> Kernel =================

void enviar_fin_quantum(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = crear_paquete(FIN_DE_QUANTUM);

    agregar_entero_a_paquete(p, ctx->pid);
    agregar_entero_a_paquete(p, ctx->pc);
    agregar_a_paquete(p, &ctx->registros, sizeof(registros_t));

    enviar_paquete(p, socket_dispatch);
    eliminar_paquete(p);
}

void enviar_fin_proceso(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = crear_paquete(FIN_DE_PROCESO);

    agregar_entero_a_paquete(p, ctx->pid);
    agregar_entero_a_paquete(p, ctx->pc);
    agregar_a_paquete(p, &ctx->registros, sizeof(registros_t));

    enviar_paquete(p, socket_dispatch);
    eliminar_paquete(p);
}

void enviar_bloqueo_io(int socket_dispatch, t_contexto_cpu* ctx)
{
    t_paquete* p = crear_paquete(BLOQUEO_IO);

    agregar_entero_a_paquete(p, ctx->pid);
    agregar_entero_a_paquete(p, ctx->pc);
    agregar_a_paquete(p, &ctx->registros, sizeof(registros_t));

    enviar_paquete(p, socket_dispatch);
    eliminar_paquete(p);
}

// ================= Recepción =================

t_contexto_cpu* recibir_contexto_cpu(int socket_dispatch)
{
    t_list* valores = recibir_paquete(socket_dispatch);

    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));
    ctx->pid = *(int*)list_get(valores, 0);
    ctx->pc  = *(int*)list_get(valores, 1);
    ctx->registros = *(registros_t*)list_get(valores, 2);

    list_destroy_and_destroy_elements(valores, free);
    return ctx;
}
