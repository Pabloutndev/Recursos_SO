
#include <stdlib.h>
#include <string.h>

#include <common/io/io_ops.h>
#include <serializacion/io.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

// ================================
// IO - SLEEP
// ================================
t_paquete* serializar_io_sleep(const t_io_sleep* req)
{
    if (!req) return NULL;

    t_paquete* p = paquete_create(OP_IO_GENERIC_SLEEP);
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
