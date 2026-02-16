#include <adaptadores/consola_kernel_adapter.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

void enviar_comando_string(int socket, op_code code, const char* str)
{
    t_paquete* p = paquete_create(code);
    paquete_write_string(p, str);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}

void enviar_comando_uint32(int socket, op_code code, uint32_t val)
{
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, val);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}

void enviar_comando_simple(int socket, op_code code)
{
    t_paquete* p = paquete_create(code);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}

void enviar_comando_run(int socket, const char* path, uint32_t prioridad)
{
    t_paquete* p = paquete_create(OP_CONSOLA_RUN);
    paquete_write_string(p, path);
    paquete_write_uint32(p, prioridad);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}
