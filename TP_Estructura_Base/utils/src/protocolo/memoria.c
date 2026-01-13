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
    enviar_paquete(p, socket_memoria);
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
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

void enviar_fin_proceso(int socket_memoria, t_mem_fin_proceso* req)
{
    t_paquete* p = serializar_mem_fin_proceso(req);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

/// TODO: REVISAR
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
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

t_mem_traducir_pagina* recibir_mem_traducir_pagina(t_paquete* p) 
{
    return deserializar_mem_traducir_pagina(p);
}

/// ==============================
/// LEER/ESCRIBIR MEMORIA
/// ==============================
void enviar_lectura_memoria(int socket_memoria, t_mem_rw* req)
{
    t_paquete* p = serializar_mem_rw(req);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

void enviar_escritura_memoria(int socket_memoria, t_mem_rw* req)
{
    t_paquete* p = serializar_mem_traducir_pagina(req);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

t_mem_rw* recibir_mem_rw(t_paquete* p)
{
    return deserializar_mem_rw(p);
}

/// ==============================
/// RESPUESTA LECTURA
/// ==============================
void enviar_respuesta_lectura(int socket_memoria, t_mem_respuesta_lectura* req)
{
    t_paquete* p = serializar_mem_respuesta_lectura(req);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

t_mem_respuesta_lectura* recibir_instruccion(t_paquete* p)
{
    return deserializar_mem_respuesta_traduccion(p);
}

/// ==============================
/// RESPUESTA TRADUCCION
/// ==============================
void enviar_respuesta_lectura(int socket_memoria, t_mem_respuesta_traduccion* req) {
    t_paquete* p = serializar_mem_respuesta_traduccion(req);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}

t_mem_respuesta_traduccion* recibir_traducir(t_paquete* p)
{
    return deserializar_mem_respuesta_lectura(p);
}
