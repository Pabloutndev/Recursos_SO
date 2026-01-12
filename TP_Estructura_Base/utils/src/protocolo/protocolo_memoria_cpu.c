#include <protocolo/protocolo_memoria_cpu.h>

#include <common/memoria/requests.h>
#include <common/memoria/responses.h>

#include <serializacion/memoria.h>
#include <deserializacion/memoria.h>

#include <paquete/paquete.h>
#include <commons/collections/list.h>

/* ===================== REQUESTS ===================== */

void enviar_traduccion_pagina(int skt, t_mem_traducir_pagina* req)
{
    t_paquete* p = serializar_mem_traducir_pagina(req);
    enviar_paquete(p, skt);
    eliminar_paquete(p);
}

void enviar_lectura_memoria(int skt, t_mem_rw* req)
{
    t_paquete* p = serializar_mem_rw(LEER_MEMORIA, req, NULL);
    enviar_paquete(p, skt);
    eliminar_paquete(p);
}

void enviar_escritura_memoria(int skt, t_mem_rw* req)
{
    t_paquete* p = serializar_mem_rw(ESCRIBIR_MEMORIA, req, data);
    enviar_paquete(p, skt);
    eliminar_paquete(p);
}

/* ===================== RESPONSES ===================== */

void enviar_respuesta_traduccion(int skt, t_mem_respuesta_traduccion* resp)
{
    t_paquete* p = serializar_mem_respuesta_traduccion(resp);
    enviar_paquete(p, skt);
    eliminar_paquete(p);
}

void enviar_respuesta_lectura(int skt, t_mem_respuesta_lectura* resp)
{
    t_paquete* p = serializar_mem_respuesta_lectura(resp);
    enviar_paquete(p, skt);
    eliminar_paquete(p);
}

void enviar_respuesta_escritura(int skt, t_mem_respuesta_lectura* resp)
{
    t_paquete* p = serializar_mem_respuesta_escritura(resp);
    enviar_paquete(p, skt);
    eliminar_paquete(p);
}

/* AUX */
t_mem_traducir_pagina recibir_traduccion_pagina(int skt)
{
    t_list* payload = recibir_paquete(skt);
    t_mem_traducir_pagina req = deserializar_mem_traducir_pagina(payload);
    list_destroy_and_destroy_elements(payload, free);
    return req;
}