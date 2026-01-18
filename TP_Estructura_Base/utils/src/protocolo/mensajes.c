// Structs compartidas
#include <model/model.h>

// net
#include <conexion/conexion.h>
#include <paquete/paquete.h>

// serializacion / deserializacion
#include <serializacion/serializacion.h>

// protocolo
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>

/// ==============================
/// CONTEXTO - PROCESO (KERNEL <-> CPU)
/// ==============================
void enviar_contexto(int socket_dispatch, t_contexto_cpu* ctx, op_code code)
{
    t_paquete* p = serializar_contexto_cpu(ctx, code);
    enviar_paquete(socket_dispatch, p);
    paquete_destroy(p);
}

t_contexto_cpu* recibir_contexto(t_paquete* p)
{
    return deserializar_contexto_cpu(p);
}

/// ==============================
/// TLB ENTRY (note: delete?)
/// ==============================
/*void enviar_tlb_entry(int socket_dest, t_tlb_entry* entry)
{
    t_paquete* p = serializar_tlb_entry(entry);
    if (!p) return;

    enviar_paquete(socket_dest, p);
    paquete_destroy(p);
}

t_tlb_entry* recibir_tlb_entry(t_paquete* p)
{
    return deserializar_tlb_entry(p);
}*/

/// ==============================
/// TLB COMPLETA (note: delete?)
/// ==============================
/*void enviar_tlb(int socket_dest, t_tlb* tlb)
{
    t_paquete* p = serializar_tlb(tlb);
    if (!p) return;

    enviar_paquete(socket_dest, p);
    paquete_destroy(p);
}

t_tlb* recibir_tlb(t_paquete* p)
{
    return deserializar_tlb(p);
}*/

/// ==============================
/// INTERRUPCION
/// ==============================
void enviar_interrupcion_cpu(int socket_interrupt)
{
    t_paquete* p = paquete_create(OP_INTERRUPCION_CPU);
    enviar_paquete(socket_interrupt, p);
    paquete_destroy(p);
}

/*
    GESTION DE STRUCTS DE MEMORIA
*/

/// ==============================
/// FETCH INSTRUCCION
/// ==============================
void enviar_fetch_instruccion(int socket_memoria, t_mem_fetch* req, op_code code)
{
    t_paquete* p = serializar_mem_fetch(req, code);
    enviar_paquete(socket_memoria, p);
    paquete_destroy(p);
}

t_mem_fetch* recibir_fetch(t_paquete* p)
{
    return deserializar_mem_fetch(p);
}

/// ==============================
/// PROCESS
/// ==============================
void enviar_init_proceso(int socket_memoria, t_mem_init_proceso* req, op_code code)
{
    t_paquete* p = serializar_mem_init_proceso(req, code);
    enviar_paquete(socket_memoria, p);
    paquete_destroy(p);
}

t_mem_init_proceso* recibir_init_proceso(t_paquete* p)
{
    return deserializar_mem_init_proceso(p);
}

void enviar_fin_proceso(int socket_memoria, t_mem_fin_proceso* req, op_code code)
{
    t_paquete* p = serializar_mem_fin_proceso(req, code);
    enviar_paquete(socket_memoria, p);
    paquete_destroy(p);
}

t_mem_fin_proceso* recibir_fin_proceso(t_paquete* p)
{
    return deserializar_mem_fin_proceso(p);
}

/// ==============================
/// TRADUCIR PAGINA
/// ==============================
void enviar_traduccion_pagina(int socket_memoria, t_mem_traducir* req, op_code code)
{
    t_paquete* p = serializar_mem_traducir_pagina(req, code);
    enviar_paquete(socket_memoria, p);
    paquete_destroy(p);
}

t_mem_traducir* recibir_mem_traducir_pagina(t_paquete* p) 
{
    return deserializar_mem_traducir_pagina(p);
}

/// ==============================
/// LEER/ESCRIBIR MEMORIA
/// ==============================
void enviar_lectura_memoria(int socket_memoria, t_mem_read* req, op_code code)
{
    t_paquete* p = serializar_mem_read(req, code);
    enviar_paquete(socket_memoria, p);
    paquete_destroy(p);
}

void enviar_escritura_memoria(int socket_memoria, t_mem_write* req, op_code code)
{
    t_paquete* p = serializar_mem_write(req, code); 
    enviar_paquete(socket_memoria, p);
    paquete_destroy(p);
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
    paquete_destroy(p);
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
    paquete_destroy(p);
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
    paquete_destroy(p);
}

char* recibir_respuesta_instruccion(t_paquete* p)
{
    return deserializar_mem_respuesta_instruccion(p);
}

/// ==============================
/// IO - SLEEP
/// ==============================
void enviar_io_sleep(int socket_io, t_io_sleep* io)
{
    t_paquete* p = serializar_io_sleep((const t_io_sleep*) io);
    enviar_paquete(socket_io, p);
    paquete_destroy(p);
}

t_io_sleep* recibir_io_sleep(t_paquete* p)
{
    return deserializar_io_sleep(p);
}

// ================================
// IO - FS WRITE
// ================================
void enviar_io_fs_write(int socket_io, t_io_fs_write* io)
{
    t_paquete* p = serializar_io_fs_write((const t_io_fs_write*) io);
    enviar_paquete(socket_io, p);
    paquete_destroy(p);
}

t_io_fs_write* recibir_io_fs_write(t_paquete* p)
{
    return deserializar_io_fs_write(p);
}

// ================================
// IO - FS CREATE
// ================================
void enviar_io_fs_create(int socket_io, t_io_fs_create* io)
{
    t_paquete* p = serializar_io_fs_create((const t_io_fs_create*) io);
    enviar_paquete(socket_io, p);
    paquete_destroy(p);
}

t_io_fs_create* recibir_io_fs_create(t_paquete* p)
{
    return deserializar_io_fs_create(p);
}

// ================================
// IO -> KERNEL (FINALIZACION)
// ================================
void enviar_io_fin(int socket_kernel, uint32_t pid, bool success)
{
    t_paquete* p = (success == true) ?
        paquete_create(OP_IO_FIN_OPERACION) : 
        paquete_create(OP_FAIL);
    paquete_write_uint32(p, pid);
    enviar_paquete(socket_kernel, p);
    paquete_destroy(p);
}

uint32_t recibir_pid_fin_io(t_paquete* p)
{
    uint32_t pid;
    if ( paquete_read_uint32(p, &pid)) {
        return pid;
    }
    return pid;
}

//
// GENERICOS
// 
void enviar_respuesta(int socket, op_code code)
{
    t_paquete* p = paquete_create(code);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}

void enviar_respuesta_ok(int socket)
{
    t_paquete* p = paquete_create(OP_OK);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}

void enviar_respuesta_fail(int socket)
{
    t_paquete* p = paquete_create(OP_FAIL);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}

bool recibir_respuesta(t_paquete* p)
{
    return p->codigo_operacion == OP_OK;
}
