#include <paquete/paquete.h>
#include <protocolo/op_code.h>

static bool buffer_write(t_buffer* b, const void* data, uint32_t size);
static bool buffer_read(t_buffer* b, void* dest, uint32_t size);

/// NOTE: API FOR SEND/RECV PACKAGE BY SOCKETS

/// NOTE: CREATE / DESTROY PACKAGE 
t_paquete* paquete_create(op_code code)
{
    t_paquete* p = malloc(sizeof(t_paquete));
    if (!p) return NULL;

    p->codigo_operacion = code;
    p->buffer = buffer_create();

    return p;
}

t_buffer* buffer_create(void)
{
    t_buffer* b = malloc(sizeof(t_buffer));
    b->size = 0;
    b->offset = 0;
    b->stream = NULL;
    return b;
}

void paquete_destroy(t_paquete* p)
{
    if (!p) return;
    free(p->buffer->stream);
    free(p->buffer);
    free(p);
}

/// NOTE: SEND / RECV
bool enviar_paquete(int fd, t_paquete* p)
{
    if (send(fd, &p->codigo_operacion, sizeof(op_code), 0) <= 0)
        return false;

    if (send(fd, &p->buffer->size, sizeof(uint32_t), 0) <= 0)
        return false;

    if (p->buffer->size > 0)
        if (send(fd, p->buffer->stream, p->buffer->size, 0) <= 0)
            return false;

    return true;
}

t_paquete* recibir_paquete(int fd)
{
    op_code code;
    uint32_t size;

    if (recv(fd, &code, sizeof(op_code), MSG_WAITALL) <= 0)
        return NULL;

    if (recv(fd, &size, sizeof(uint32_t), MSG_WAITALL) <= 0)
        return NULL;

    t_paquete* p = paquete_create(code);
    if (!p) return NULL;

    if (size > 0) {
        p->buffer->stream = malloc(size);
        p->buffer->size = size;
        recv(fd, p->buffer->stream, size, MSG_WAITALL);
    }

    return p;
}

op_code devolver_operacion(t_paquete* p)
{
    return p->codigo_operacion;
}

/// NOTE: SERIALIZACION / DESERIALIZACION
bool paquete_write_string(t_paquete* p, const char* s)
{
    uint32_t len = strlen(s) + 1;
    return buffer_write(p->buffer, &len, sizeof(uint32_t)) &&
           buffer_write(p->buffer, s, len);
}

char* paquete_read_string(t_paquete* p)
{
    uint32_t len;
    if (!buffer_read(p->buffer, &len, sizeof(uint32_t)))
        return NULL;

    char* s = malloc(len);
    if (!buffer_read(p->buffer, s, len)) {
        free(s);
        return NULL;
    }
    return s;
}

bool paquete_write_buffer(t_paquete* p, const void* data, uint32_t size)
{
    return buffer_write(p->buffer, &size, sizeof(uint32_t)) &&
           buffer_write(p->buffer, data, size);
}

void* paquete_read_buffer(t_paquete* p, uint32_t* size)
{
    if (!buffer_read(p->buffer, size, sizeof(uint32_t)))
        return NULL;

    void* data = malloc(*size);
    if (!buffer_read(p->buffer, data, *size)) {
        free(data);
        return NULL;
    }
    return data;
}

bool paquete_write_uint8(t_paquete* p, uint8_t v)
{
    return buffer_write(p->buffer, &v, sizeof(uint8_t));
}

bool paquete_read_uint8(t_paquete* p, uint8_t* v)
{
    return buffer_read(p->buffer, v, sizeof(uint8_t));
}

bool paquete_write_uint32(t_paquete* p, uint32_t v)
{
    return buffer_write(p->buffer, &v, sizeof(uint32_t));
}

bool paquete_read_uint32(t_paquete* p, uint32_t* v)
{
    return buffer_read(p->buffer, v, sizeof(uint32_t));
}

bool paquete_write_int(t_paquete* p, int v)
{
    return buffer_write(p->buffer, &v, sizeof(int));
}

bool paquete_read_int(t_paquete* p, int* v)
{
    return buffer_read(p->buffer, v, sizeof(int));
}

bool paquete_write_float(t_paquete* p, float v)
{
    return buffer_write(p->buffer, &v, sizeof(float));
}

bool paquete_read_float(t_paquete* p, float* v)
{
    return buffer_read(p->buffer, v, sizeof(float));
}

bool paquete_write_bool(t_paquete* p, bool v)
{
    return buffer_write(p->buffer, &v, sizeof(bool));
}

bool paquete_read_bool(t_paquete* p, bool* v)
{
    return buffer_read(p->buffer, v, sizeof(bool));
}

/// NOTE: FUNCIONES AUXILIARES 
static bool buffer_write(t_buffer* b, const void* data, uint32_t size)
{
    void* new_stream = realloc(b->stream, b->size + size);
    if (!new_stream) return false;

    memcpy(new_stream + b->size, data, size);
    b->stream = new_stream;
    b->size += size;
    return true;
}

static bool buffer_read(t_buffer* b, void* dest, uint32_t size)
{
    if (b->offset + size > b->size) return false;

    memcpy(dest, b->stream + b->offset, size);
    b->offset += size;
    return true;
}
