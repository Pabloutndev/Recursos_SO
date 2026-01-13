#include <serializacion/memoria.h>
#include <common/memoria/memoria.h>

/// ##### REQUESTS - SERIALIZACION #####

t_paquete* serializar_mem_init_proceso(t_mem_init_proceso* req) {
    t_paquete* p = paquete_create(OP_INIT_PROCESO);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->tamanio);
    return p;
}

t_mem_init_proceso* deserializar_mem_init_proceso(t_paquete* p) {
    t_mem_init_proceso* req = malloc(sizeof(t_mem_init_proceso));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->tamanio);
    return req;
}

t_paquete* serializar_mem_fin_proceso(t_mem_fin_proceso* req) {
    t_paquete* p = paquete_create(OP_FIN_PROCESO);
    paquete_write_uint32(p, req->pid);
    return p;
}

t_mem_fin_proceso* deserializar_mem_fin_proceso(t_paquete* p) {
    t_mem_fin_proceso* req = malloc(sizeof(t_mem_fin_proceso));
    paquete_read_uint32(p, &req->pid);
    return req;
}

t_paquete* serializar_mem_traducir_pagina(t_mem_traducir_pagina* req) {
    t_paquete* p = paquete_create(OP_ACCESO_TABLA);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->pagina);
    return p;
}

t_mem_traducir_pagina* deserializar_mem_traducir_pagina(t_paquete* p) {
    t_mem_traducir_pagina* req = malloc(sizeof(t_mem_traducir_pagina));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->pagina);
    return req;
}

t_paquete* serializar_mem_read(t_mem_read* req) {
    t_paquete* p = paquete_create(OP_LEER_MEMORIA);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->direccion_fisica);
    paquete_write_uint32(p, req->tamanio);
    return p;
}

t_mem_read* deserializar_mem_read(t_paquete* p) {
    t_mem_read* req = malloc(sizeof(t_mem_read));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->direccion_fisica);
    paquete_read_uint32(p, &req->tamanio);
    return req;
}

t_paquete* serializar_mem_write(t_mem_write* req) {
    t_paquete* p = paquete_create(OP_ESCRIBIR_MEMORIA);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->direccion_fisica);
    paquete_write_buffer(p, req->buffer, req->tamanio);
    return p;
}

t_mem_write* deserializar_mem_write(t_paquete* p) {
    t_mem_write* req = malloc(sizeof(t_mem_write));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->direccion_fisica);
    req->buffer = paquete_read_buffer(p, &req->tamanio);
    return req;
}

t_paquete* serializar_mem_fetch(t_mem_fetch* req) {
    t_paquete* p = paquete_create(OP_FETCH_INSTRUCCION);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->pc);
    return p;
}

t_mem_fetch* deserializar_mem_fetch(t_paquete* p) {
    t_mem_fetch* req = malloc(sizeof(t_mem_fetch));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->pc);
    return req;
}

/// ##### RESPONSES - DESERIALIZACION #####

t_paquete* serializar_mem_respuesta_traduccion(t_mem_respuesta_traduccion* res) {
    t_paquete* p = paquete_create(OP_RESPUESTA_TRADUCCION);
    paquete_write_bool(p, res->ok);
    paquete_write_uint32(p, res->frame);
    return p;
}

t_mem_respuesta_traduccion* deserializar_mem_respuesta_traduccion(t_paquete* p) {
    t_mem_respuesta_traduccion* res = malloc(sizeof(t_mem_respuesta_traduccion));
    paquete_read_bool(p, &res->ok);
    paquete_read_uint32(p, &res->frame);
    return res;
}

t_paquete* serializar_mem_respuesta_lectura(t_mem_respuesta_lectura* res) {
    t_paquete* p = paquete_create(OP_RESPUESTA_LECTURA);
    paquete_write_bool(p, res->ok);
    paquete_write_buffer(p, res->data, res->size);
    return p;
}

t_mem_respuesta_lectura* deserializar_mem_respuesta_lectura(t_paquete* p) {
    t_mem_respuesta_lectura* res = malloc(sizeof(t_mem_respuesta_lectura));
    paquete_read_bool(p, &res->ok);
    res->data = paquete_read_buffer(p, &res->size);
    return res;
}
