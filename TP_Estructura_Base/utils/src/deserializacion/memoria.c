#include <deserializacion/memoria.h>
#include <stdlib.h>

t_mem_init_proceso deserializar_mem_init_proceso(t_list* payload)
{
    return (t_mem_init_proceso){
        .pid = *(uint32_t*)list_get(payload, 0),
        .tamanio = *(uint32_t*)list_get(payload, 1)
    };
}

t_mem_fin_proceso deserializar_mem_fin_proceso(t_list* payload)
{
    return (t_mem_fin_proceso){
        .pid = *(uint32_t*)list_get(payload, 0)
    };
}

t_mem_traducir_pagina deserializar_mem_traducir_pagina(t_list* l)
{
    return (t_mem_traducir_pagina) {
        .pid = *(uint32_t*)list_get(l,0),
        .pagina = *(uint32_t*)list_get(l,1)
    };
}

t_mem_rw deserializar_mem_rw(t_list* l, void** data)
{
    t_mem_rw r = {
        .pid = *(uint32_t*)list_get(l,0),
        .direccion_fisica = *(uint32_t*)list_get(l,1),
        .tamanio = *(uint32_t*)list_get(l,2)
    };

    if (data && list_size(l) > 3)
        *data = list_get(l,3);

    return r;
}

/* ========== RESPUESTAS ========== */

t_mem_respuesta_traduccion deserializar_mem_respuesta_traduccion(t_list* l)
{
    return (t_mem_respuesta_traduccion){
        .ok = *(bool*)list_get(l,0),
        .frame = *(uint32_t*)list_get(l,1)
    };
}

t_mem_respuesta_lectura deserializar_mem_respuesta_lectura(t_list* l)
{
    t_mem_respuesta_lectura r = {
        .ok = *(bool*)list_get(l,0),
        .size = *(uint32_t*)list_get(l,1),
        .data = NULL
    };

    if (r.ok && list_size(l) > 2)
        r.data = list_get(l,2);

    return r;
}

bool deserializar_mem_respuesta_escritura(t_list* payload)
{
    return *(bool*)list_get(payload, 0);
}

bool deserializar_mem_respuesta_kernel(t_list* payload)
{
    return *(bool*)list_get(payload, 0);
}