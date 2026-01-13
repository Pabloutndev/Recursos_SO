#include <common/cpu/cpu.h>
#include <common/memoria/memoria.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <serializacion/memoria.h>
#include <protocolo/memoria.h>

/// ==============================
/// FETCH INSTRUCCION
/// ==============================
void enviar_fetch_instruccion(int socket_memoria, t_mem_fetch* req)
{
    t_paquete* p = serializar_mem_fetch(req);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

t_mem_fetch* recibir_fetch(t_paquete* p)
{
    return deserializar_mem_fetch(p);
}

/// ==============================
/// PROCESS
/// ==============================
void enviar_init_proceso(int socket_memoria, t_mem_init_proceso* req)
{
    t_paquete* p = serializar_mem_init_proceso(req);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

void enviar_fin_proceso(int socket_memoria, t_mem_fin_proceso* req)
{
    t_paquete* p = serializar_mem_fin_proceso(req);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

bool recibir_respuesta_kernel(int socket_memoria)
{
    t_list* payload = recibir_paquete(socket_memoria);
    bool ok = *(bool*)list_get(payload, 0);

    list_destroy_and_destroy_elements(payload, free);
    return ok;
}

/// ==============================
/// TRADUCIR PAGINA
/// ==============================
void enviar_traduccion_pagina(int socket_memoria, t_mem_traducir_pagina* req)
{
    t_paquete* p = serializar_mem_traducir_pagina(req);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

t_mem_traducir_pagina* recibir_mem_traducir_pagina(t_paquete* p) 
{
    return deserializar_mem_traducir_pagina(p);
}

/// ==============================
/// LEER/ESCRIBIR MEMORIA
/// ==============================
void enviar_lectura_memoria(int socket_memoria, t_mem_read* req)
{
    t_paquete* p = serializar_mem_read(req);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

void enviar_escritura_memoria(int socket_memoria, t_mem_write* req)
{
    t_paquete* p = serializar_mem_write(req); 
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

t_mem_read* recibir_lectura_memoria(t_paquete* p)
{
    return deserializar_mem_read(p);
}

t_mem_write* recibir_escritura_memoria(t_paquete* p)
{
    return deserializar_mem_write(p);
}

/// ==============================
/// RESPUESTA LECTURA
/// ==============================
void enviar_respuesta_lectura(int socket_memoria, t_mem_respuesta_lectura* req)
{
    t_paquete* p = serializar_mem_respuesta_lectura(req);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

t_mem_respuesta_lectura* recibir_respuesta_lectura(t_paquete* p)
{
    return deserializar_mem_respuesta_lectura(p);
}

/// ==============================
/// RESPUESTA TRADUCCION
/// ==============================
void enviar_respuesta_traduccion(int socket_memoria, t_mem_respuesta_traduccion* req) {
    t_paquete* p = serializar_mem_respuesta_traduccion(req);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

t_mem_respuesta_traduccion* recibir_respuesta_traduccion(t_paquete* p)
{
    return deserializar_mem_respuesta_traduccion(p);
}
