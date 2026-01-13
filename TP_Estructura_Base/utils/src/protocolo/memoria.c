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

void enviar_respuesta_kernel(int socket_memoria, bool ok) {
    // OpCode generic response or dedicated?
    // Using simple packet with bool.
    // Assuming receiver just checks payload.
    // The OpCode in header matters for switching, but here kernel waits for ANY packet and checks content?
    // recibir_respuesta_kernel implementation just calls recibir_paquete.
    // recibir_paquete doesn't check OpCode?
    // It returns payload list.
    // So OpCode doesn't theoretically matter if Kernel is just blocked on recv.
    // But let's use a standard one.
    t_paquete* p = paquete_create(OP_RESPUESTA_GENERICA); // Reuse or defined?
    // Define OP_RESPUESTA_GENERICA in op_code if needed, or use OP_OK implies success?
    // If I use OP_OK, it might be confusing if payload is bool.
    // Let's use OP_RES_GENERAL.
    // I need to check op_code.h.
    // For now I'll use OP_PROCESO_EXIT (msg) or similar? No.
    // Let's use a generic value or assume OP_INIT_PROCESO response has same opcode? No.
    // I will use OP_DEBUG for now or just add it.
    // Actually, let's look at `recibir_respuesta_kernel` again.
    // It reads a BOOL.
    paquete_write_bool(p, ok);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
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
/// ==============================
/// RESPUESTA INSTRUCCION 
/// ==============================
void enviar_respuesta_instruccion(int socket_memoria, char* instruccion) {
    t_paquete* p = serializar_mem_respuesta_instruccion(instruccion);
    enviar_paquete(socket_memoria, p);
    eliminar_paquete(p);
}

char* recibir_respuesta_instruccion(t_paquete* p)
{
    return deserializar_mem_respuesta_instruccion(p);
}
