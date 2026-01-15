#include <common/memoria/memoria.h>
#include <common/cpu/contexto.h>
#include <common/kernel/kernel.h>
#include <common/io/io_ops.h>

#include <paquete/paquete.h>
#include <serializacion/serializacion.h>
#include <protocolo/op_code.h>

/// ===========
/// KERNEL Y CPU
/// ===========

/// ##### REQUESTS - SERIALIZACION y RESPONSES - DESERIALIZACION #####

t_paquete* serializar_contexto_cpu(t_contexto_cpu* ctx, op_code code) {
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, ctx->pid);
    paquete_write_uint32(p, ctx->pc);
    paquete_write_uint32(p, ctx->quantum);
    paquete_write_uint32(p, ctx->finalizado);
    paquete_write_uint32(p, ctx->bloqueado);
    paquete_write_uint32(p, ctx->io_time);
    
    // Serializar registros
    serializar_registros(p, &ctx->registros);
    return p;
}

t_contexto_cpu* deserializar_contexto_cpu(t_paquete* p) {
    t_contexto_cpu* ctx = malloc(sizeof(t_contexto_cpu));
    paquete_read_uint32(p, &ctx->pid);
    paquete_read_uint32(p, &ctx->pc);
    paquete_read_uint32(p, &ctx->quantum);
    paquete_read_uint32(p, &ctx->finalizado);
    paquete_read_uint32(p, &ctx->bloqueado);
    paquete_read_uint32(p, &ctx->io_time);

    deserializar_registros(p, &ctx->registros);
    return ctx;
}

t_paquete* serializar_process(t_process* proc) {
    t_paquete* p = paquete_create(OP_PROCESO_EXEC);
    paquete_write_int(p, proc->pid);
    paquete_write_int(p, proc->quantum);
    serializar_registros(&p, &(proc->registros));
    return p;
}

t_process* deserializar_process(t_paquete* p) {
    t_process* proc = malloc(sizeof(t_process));
    paquete_read_int(p, &proc->pid);
    paquete_read_int(p, &proc->quantum);
    deserializar_registros(p, &proc->registros);
    return proc;
}

t_paquete* serializar_fin_quatum(t_process* proc) {
    t_paquete* p = paquete_create(OP_FIN_DE_QUANTUM);
    paquete_write_int(p, proc->pid);
    paquete_write_int(p, proc->quantum);
    serializar_registros(p, &proc->registros);
    return p;
}

/// ##### AUXILIAR SERIALIZACION/DESERIALIZACION DE REGISTROS

void serializar_registros(t_paquete* p, registros_t* r) {
    // 8 bits
    paquete_write_uint8(p, r->AX);
    paquete_write_uint8(p, r->BX);
    paquete_write_uint8(p, r->CX);
    paquete_write_uint8(p, r->DX);

    // 32 bits
    paquete_write_uint32(p, r->EAX);
    paquete_write_uint32(p, r->EBX);
    paquete_write_uint32(p, r->ECX);
    paquete_write_uint32(p, r->EDX);
    paquete_write_uint32(p, r->SI);
    paquete_write_uint32(p, r->DI);
    paquete_write_uint32(p, r->PC);
}

void deserializar_registros(t_paquete* p, registros_t* r) {
    // 8 bits
    paquete_read_uint8(p, &r->AX);
    paquete_read_uint8(p, &r->BX);
    paquete_read_uint8(p, &r->CX);
    paquete_read_uint8(p, &r->DX);

    // 32 bits
    paquete_read_uint32(p, &r->EAX);
    paquete_read_uint32(p, &r->EBX);
    paquete_read_uint32(p, &r->ECX);
    paquete_read_uint32(p, &r->EDX);
    paquete_read_uint32(p, &r->SI);
    paquete_read_uint32(p, &r->DI);
    paquete_read_uint32(p, &r->PC);
}

/// ===========
/// MEMORIA
/// ===========

/// ##### REQUESTS - SERIALIZACION #####

t_paquete* serializar_mem_init_proceso(t_mem_init_proceso* req, op_code code) {
    t_paquete* p = paquete_create(code);
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

t_paquete* serializar_mem_fin_proceso(t_mem_fin_proceso* req, op_code code) {
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, req->pid);
    return p;
}

t_mem_fin_proceso* deserializar_mem_fin_proceso(t_paquete* p) {
    t_mem_fin_proceso* req = malloc(sizeof(t_mem_fin_proceso));
    paquete_read_uint32(p, &req->pid);
    return req;
}

t_paquete* serializar_mem_traducir_pagina(t_mem_traducir* req, op_code code) {
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->direccion_logica);
    return p;
}

t_mem_traducir* deserializar_mem_traducir_pagina(t_paquete* p) {
    t_mem_traducir* req = malloc(sizeof(t_mem_traducir));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->direccion_logica);
    return req;
}

t_paquete* serializar_mem_read(t_mem_read* req, op_code code) {
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->direccion_logica);
    paquete_write_uint32(p, req->size);
    return p;
}

t_mem_read* deserializar_mem_read(t_paquete* p) {
    t_mem_read* req = malloc(sizeof(t_mem_read));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->direccion_logica);
    paquete_read_uint32(p, &req->size);
    return req;
}

t_paquete* serializar_mem_write(t_mem_write* req, op_code code) {
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->direccion_logica);
    paquete_write_buffer(p, req->buffer, req->size);
    return p;
}

t_mem_write* deserializar_mem_write(t_paquete* p) {
    t_mem_write* req = malloc(sizeof(t_mem_write));
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->direccion_logica);
    req->buffer = paquete_read_buffer(p, &req->size);
    return req;
}

t_paquete* serializar_mem_fetch(t_mem_fetch* req, op_code code) {
    t_paquete* p = paquete_create(code);
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
    t_paquete* p = paquete_create(OP_MEM_RESP_TRADUCCION);
    paquete_write_bool(p, res->ok);
    paquete_write_uint32(p, res->direccion_fisica);
    return p;
}

t_mem_respuesta_traduccion* deserializar_mem_respuesta_traduccion(t_paquete* p) {
    t_mem_respuesta_traduccion* res = malloc(sizeof(t_mem_respuesta_traduccion));
    paquete_read_bool(p, &res->ok);
    paquete_read_uint32(p, &res->direccion_fisica);
    return res;
}

t_paquete* serializar_mem_respuesta_lectura(t_mem_respuesta_lectura* res) {
    t_paquete* p = paquete_create(OP_MEM_RESP_LECTURA);
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

t_paquete* serializar_mem_respuesta_instruccion(char* instruccion) {
    t_paquete* p = paquete_create(OP_MEM_FETCH_INSTRUCCION);
    paquete_write_string(p, instruccion);
    return p;
}

char* deserializar_mem_respuesta_instruccion(t_paquete* p) {
    return paquete_read_string(p);
}

// ================================
// IO - SLEEP
// ================================
t_paquete* serializar_io_sleep(const t_io_sleep* req)
{
    if (!req) return NULL;

    t_paquete* p = paquete_create(OP_IO_SLEEP);
    if (!p) return NULL;

    if (!paquete_write_uint32(p, req->pid)) { paquete_destroy(p); return NULL; }
    if (!paquete_write_uint32(p, req->tiempo)) { paquete_destroy(p); return NULL; }

    return p;
}

t_io_sleep* deserializar_io_sleep(t_paquete* p)
{
    if (!p) return NULL;

    t_io_sleep* req = malloc(sizeof(t_io_sleep));
    if (!req) return NULL;

    if (!paquete_read_uint32(p, &req->pid)) { free(req); return NULL; }
    if (!paquete_read_uint32(p, &req->tiempo)) { free(req); return NULL; }

    return req;
}

// ================================
// IO - FS WRITE
// ================================
t_paquete* serializar_io_fs_write(const t_io_fs_write* req)
{
    if (!req) return NULL;

    t_paquete* p = paquete_create(OP_IO_FS_WRITE);
    if (!p) return NULL;

    // pid
    if (!paquete_write_uint32(p, req->pid)) { paquete_destroy(p); return NULL; }

    // path[256] como buffer (se antepone uint32 size); usamos el contenido actual
    // Enviamos hasta el primer '\0' + 1 para incluir terminador (o todo 256 si preferís fijo)
    size_t path_len = strnlen(req->path, sizeof(req->path));
    // incluimos terminador (si hay lugar)
    size_t to_send = (path_len < sizeof(req->path)) ? path_len + 1 : sizeof(req->path);

    if (!paquete_write_buffer(p, req->path, (uint32_t)to_send)) { paquete_destroy(p); return NULL; }

    // offset y size
    if (!paquete_write_uint32(p, req->offset)) { paquete_destroy(p); return NULL; }
    if (!paquete_write_uint32(p, req->size))   { paquete_destroy(p); return NULL; }

    return p;
}

t_io_fs_write* deserializar_io_fs_write(t_paquete* p)
{
    if (!p) return NULL;

    t_io_fs_write* req = malloc(sizeof(t_io_fs_write));
    if (!req) return NULL;

    // pid
    if (!paquete_read_uint32(p, &req->pid)) { free(req); return NULL; }

    // path buffer
    uint32_t path_size = 0;
    void* path_buf = paquete_read_buffer(p, &path_size);
    if (!path_buf) { free(req); return NULL; }

    // Validar tamaño y copiar asegurando terminación
    if (path_size == 0) {
        // string vacía
        req->path[0] = '\0';
    } else if (path_size <= sizeof(req->path)) {
        // cabe completo en path[256]
        memcpy(req->path, path_buf, path_size);
        // Garantizar '\0' final si viniera sin terminador
        if (req->path[path_size - 1] != '\0') {
            // si no estaba terminado, forcemos:
            if (path_size < sizeof(req->path)) {
                req->path[path_size] = '\0';
            } else {
                // último char
                req->path[sizeof(req->path) - 1] = '\0';
            }
        }
        // Relleno el resto con 0 para evitar basura
        if (path_size < sizeof(req->path)) {
            memset(req->path + path_size, 0, sizeof(req->path) - path_size);
        }
    } else {
        // Si el nombre es más largo que 256, truncamos y aseguramos '\0'
        memcpy(req->path, path_buf, sizeof(req->path) - 1);
        req->path[sizeof(req->path) - 1] = '\0';
    }
    free(path_buf);

    // offset y size
    if (!paquete_read_uint32(p, &req->offset)) { free(req); return NULL; }
    if (!paquete_read_uint32(p, &req->size))   { free(req); return NULL; }

    return req;
}

// ================================
// IO - FS CREATE
// ================================
t_paquete* serializar_io_fs_create(const t_io_fs_create* req)
{
    if (!req) return NULL;

    t_paquete* p = paquete_create(OP_IO_FS_CREATE);
    if (!p) return NULL;

    if (!paquete_write_uint32(p, req->pid)) { paquete_destroy(p); return NULL; }

    size_t path_len = strnlen(req->path, sizeof(req->path));
    size_t to_send = (path_len < sizeof(req->path)) ? path_len + 1 : sizeof(req->path);

    if (!paquete_write_buffer(p, req->path, (uint32_t)to_send)) { paquete_destroy(p); return NULL; }

    return p;
}

t_io_fs_create* deserializar_io_fs_create(t_paquete* p)
{
    if (!p) return NULL;

    t_io_fs_create* req = malloc(sizeof(t_io_fs_create));
    if (!req) return NULL;

    if (!paquete_read_uint32(p, &req->pid)) { free(req); return NULL; }

    uint32_t path_size = 0;
    void* path_buf = paquete_read_buffer(p, &path_size);
    if (!path_buf) { free(req); return NULL; }

    if (path_size == 0) {
        req->path[0] = '\0';
    } else if (path_size <= sizeof(req->path)) {
        memcpy(req->path, path_buf, path_size);
        if (req->path[path_size - 1] != '\0') {
            if (path_size < sizeof(req->path)) {
                req->path[path_size] = '\0';
            } else {
                req->path[sizeof(req->path) - 1] = '\0';
            }
        }
        if (path_size < sizeof(req->path)) {
            memset(req->path + path_size, 0, sizeof(req->path) - path_size);
        }
    } else {
        memcpy(req->path, path_buf, sizeof(req->path) - 1);
        req->path[sizeof(req->path) - 1] = '\0';
    }
    free(path_buf);

    return req;
}
