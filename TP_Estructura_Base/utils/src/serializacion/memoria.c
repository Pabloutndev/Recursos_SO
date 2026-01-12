#include <serializacion/memoria.h>

t_paquete* serializar_mem_traducir_pagina(t_mem_traducir_pagina* req)
{
    t_paquete* p = crear_paquete(ACCESO_TABLA);
    agregar_a_paquete(p, &req->pid, sizeof(uint32_t));
    agregar_a_paquete(p, &req->pagina, sizeof(uint32_t));
    return p;
}

t_paquete* serializar_mem_rw(op_code code, t_mem_rw* req, void* data)
{
    t_paquete* p = crear_paquete(code);
    agregar_a_paquete(p, &req->pid, sizeof(uint32_t));
    agregar_a_paquete(p, &req->direccion_fisica, sizeof(uint32_t));
    agregar_a_paquete(p, &req->tamanio, sizeof(uint32_t));

    if (code == ESCRIBIR_MEMORIA && data != NULL) {
        agregar_a_paquete(p, data, req->tamanio);
    }

    return p;
}

/* ================= RESPUESTAS ================= */

t_paquete* serializar_mem_respuesta_traduccion(t_mem_respuesta_traduccion* resp)
{
    t_paquete* p = crear_paquete(RESPUESTA_TRADUCCION);
    agregar_a_paquete(p, &resp->ok, sizeof(bool));
    agregar_a_paquete(p, &resp->frame, sizeof(uint32_t));
    return p;
}

t_paquete* serializar_mem_respuesta_lectura(t_mem_respuesta_lectura* resp)
{
    t_paquete* p = crear_paquete(RESPUESTA_LECTURA);
    agregar_a_paquete(p, &resp->ok, sizeof(bool));
    agregar_a_paquete(p, &resp->size, sizeof(uint32_t));

    if (resp->ok && resp->data)
        agregar_a_paquete(p, resp->data, resp->size);

    return p;
}

t_paquete* serializar_mem_respuesta_escritura(t_mem_respuesta_lectura* resp)
{
    t_paquete* p = crear_paquete(RESPUESTA_ESCRITURA);
    agregar_a_paquete(p, &resp->ok, sizeof(bool));
    agregar_a_paquete(p, &resp->size, sizeof(uint32_t));

    if (resp->ok && resp->data)
        agregar_a_paquete(p, resp->data, resp->size);

    return p;
}
