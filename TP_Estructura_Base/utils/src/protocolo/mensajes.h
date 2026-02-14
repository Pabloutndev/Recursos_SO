#ifndef MENSAJES_H_
#define MENSAJES_H_

#include <stdbool.h>
#include <stdint.h>

// ==============================
// Paquete
// ==============================
#include <paquete/paquete.h>

// ==============================
// Structs compartidas
// ==============================
#include <model/model.h>

// ==============================
// Net / protocolo base
// ==============================
#include <protocolo/op_code.h>

//
// =======================================================
// CPU <-> KERNEL
// =======================================================
//

void enviar_contexto(int socket_dispatch, t_contexto_cpu* ctx, op_code code);
t_contexto_cpu* recibir_contexto(t_paquete* p);

//
// ------------------------------
// TLB (opcional / experimental)
// ------------------------------
//
/*
void enviar_tlb_entry(int socket_dest, t_tlb_entry* entry);
t_tlb_entry* recibir_tlb_entry(t_paquete* p);

void enviar_tlb(int socket_dest, t_tlb* tlb);
t_tlb* recibir_tlb(t_paquete* p);
*/
//
// ------------------------------
// Interrupciones
// ------------------------------
//

void enviar_interrupcion_cpu(int socket_interrupt);


//
// =======================================================
// CPU / KERNEL <-> MEMORIA
// =======================================================
//

//
// ------------------------------
// Fetch instrucción
// ------------------------------
//

void enviar_fetch_instruccion(int socket_memoria, t_mem_fetch* req, op_code code);
t_mem_fetch* recibir_fetch(t_paquete* p);

//
// ------------------------------
// Gestión de procesos
// ------------------------------
//

void enviar_init_proceso(int socket_memoria, t_mem_init_proceso* req, op_code code);
t_mem_init_proceso* recibir_init_proceso(t_paquete* p);

void enviar_fin_proceso(int socket_memoria, t_mem_fin_proceso* req, op_code code);
t_mem_fin_proceso* recibir_fin_proceso(t_paquete* p);

//
// ------------------------------
// Traducción de páginas
// ------------------------------
//

void enviar_traduccion_pagina(int socket_memoria, t_mem_traducir* req, op_code code);
t_mem_traducir* recibir_mem_traducir_pagina(t_paquete* p);

//
// ------------------------------
// Lectura / Escritura de memoria
// ------------------------------
//

void enviar_lectura_memoria(int socket_memoria, t_mem_read* req, op_code code);
void enviar_escritura_memoria(int socket_memoria, t_mem_write* req, op_code code);

t_mem_read* recibir_lectura_memoria(t_paquete* p);
t_mem_write* recibir_escritura_memoria(t_paquete* p);

//
// ------------------------------
// Respuestas de memoria
// ------------------------------
//

void enviar_respuesta_lectura(
    int socket_memoria,
    t_mem_respuesta_lectura* req
);
t_mem_respuesta_lectura* recibir_respuesta_lectura(t_paquete* p);

void enviar_respuesta_traduccion(
    int socket_memoria,
    t_mem_respuesta_traduccion* req
);
t_mem_respuesta_traduccion* recibir_respuesta_traduccion(t_paquete* p);

void enviar_respuesta_instruccion(int socket_memoria, char* instruccion);
char* recibir_respuesta_instruccion(t_paquete* p);


//
// =======================================================
// KERNEL <-> IO
// =======================================================
//

void enviar_io_sleep(int socket_io, t_io_sleep* io);
t_io_sleep* recibir_io_sleep(t_paquete* p);

void enviar_io_fs_write(int socket_io, t_io_fs_write* io);
t_io_fs_write* recibir_io_fs_write(t_paquete* p);

void enviar_io_fs_create(int socket_io, t_io_fs_create* io);
t_io_fs_create* recibir_io_fs_create(t_paquete* p);

//
// ------------------------------
// IO - STDIN / STDOUT
// ------------------------------
//

void enviar_io_stdin_read(int socket_io, t_io_stdin_read* io);
t_io_stdin_read* recibir_io_stdin_read(t_paquete* p);

void enviar_io_stdout_write(int socket_io, t_io_stdout_write* io);
t_io_stdout_write* recibir_io_stdout_write(t_paquete* p);

//
// ------------------------------
// Memoria - Resize
// ------------------------------
//

void enviar_resize(int socket_memoria, t_mem_resize* req, op_code code);
t_mem_resize* recibir_resize(t_paquete* p);

//
// ------------------------------
// Finalización IO -> Kernel
// ------------------------------
//

void enviar_io_fin(int socket_kernel, uint32_t pid, bool success);
uint32_t recibir_pid_fin_io(t_paquete* p);


//
// =======================================================
// Respuestas genéricas
// =======================================================
//

void enviar_respuesta(int socket, op_code code);
void enviar_respuesta_ok(int socket);
void enviar_respuesta_fail(int socket);

bool recibir_respuesta(t_paquete* p);

#endif /* MENSAJES_H_ */
