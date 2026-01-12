#include <protocolo/protocolo_kernel_memoria.h>

#include <serializacion/memoria.h>
#include <deserializacion/memoria.h>

#include <paquete/paquete.h>
#include <commons/collections/list.h>
#include <stdbool.h>

/* ===================== REQUESTS ===================== */

void enviar_init_proceso(int skt_memoria, t_mem_init_proceso* req)
{
    t_paquete* p = serializar_mem_init_proceso(req);
    enviar_paquete(p, skt_memoria);
    eliminar_paquete(p);
}

void enviar_fin_proceso(int skt_memoria, t_mem_fin_proceso* req)
{
    t_paquete* p = serializar_mem_fin_proceso(req);
    enviar_paquete(p, skt_memoria);
    eliminar_paquete(p);
}

/* ===================== RESPONSES ===================== */

bool recibir_respuesta_kernel(int skt_memoria)
{
    t_list* payload = recibir_paquete(skt_memoria);
    bool ok = *(bool*)list_get(payload, 0);

    list_destroy_and_destroy_elements(payload, free);
    return ok;
}
