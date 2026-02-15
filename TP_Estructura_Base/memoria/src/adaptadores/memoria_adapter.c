#include "memoria_adapter.h"

#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/string.h>

#include <mod_memoria.h>
#include <configs/memoria_config.h>
#include <gestion/memoria_core.h>
#include <gestion/esquema_memoria.h>
#include <frames/frames.h>
#include <gestion/memoria_ram.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <adaptadores/memoria_adapter.h>

#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern t_log* logger;
extern t_log* loggerError;
extern t_memoria_config* memoria_config;

/* ========================================
 * REQUEST HANDLERS (ADAPTADORES) - ATENCION DE MENSAJES
 * ======================================== */

void memoria_adapter_atender_init_proceso(int fd, t_paquete* paquete)
{
    t_mem_init_proceso* req = recibir_init_proceso(paquete);
    if (!req) {
        log_error(loggerError, "Init proceso request NULL");
        enviar_respuesta_fail(fd);
        return;
    }

    log_info(logger, "PID: %u - Init proceso %s", req->pid, req->path);

    /* memoria_crear_proceso carga instrucciones, crea paginacion y escribe a paginas */
    if (!memoria_crear_proceso(req->pid, req->path)) {
        log_error(loggerError, "PID: %u - Fallo init proceso", req->pid);
        enviar_respuesta_fail(fd);
        free(req);
        return;
    }

    log_info(logger, "PID: %u - Proceso inicializado", req->pid);
    enviar_respuesta_ok(fd);
    free(req);
}

void memoria_adapter_atender_fin_proceso(int fd, t_paquete* paquete)
{
    t_mem_fin_proceso* req = recibir_fin_proceso(paquete);
    if (!req) {
        log_error(loggerError, "Fin proceso request NULL");
        return;
    }

    log_info(logger, "PID: %u - Fin proceso", req->pid);

    /* Liberar estructuras de memoria */
    esquema_destruir_proceso(req->pid);
    memoria_destruir_proceso(req->pid);

    log_info(logger, "PID: %u - Proceso eliminado", req->pid);
    enviar_respuesta_ok(fd);
    free(req);
}

void memoria_adapter_atender_traducir_pagina(int fd, t_paquete* paquete)
{
    t_mem_traducir* req = recibir_mem_traducir_pagina(paquete);
    t_mem_respuesta_traduccion resp = { .ok = false, .direccion_fisica = 0 };

    if (!req) {
        log_error(loggerError, "Traducir request NULL");
        enviar_respuesta_traduccion(fd, &resp);
        return;
    }

    log_info(logger, "PID: %u - Traducir dir=%u", req->pid, req->direccion_logica);

    int64_t dir_fisica = esquema_traducir(req->pid, req->direccion_logica);

    if (dir_fisica < 0) {
        log_error(loggerError, "PID: %u - Traduccion fallo dir=%u", req->pid, req->direccion_logica);
        enviar_respuesta_traduccion(fd, &resp);
        free(req);
        return;
    }

    resp.ok = true;
    resp.direccion_fisica = (uint32_t)dir_fisica;
    enviar_respuesta_traduccion(fd, &resp);
    free(req);
}

void memoria_adapter_atender_fetch_instruccion(int fd, t_paquete* paquete) {
    t_mem_fetch* req = recibir_fetch(paquete);
    if (!req) {
        log_error(loggerError, "Fetch request NULL");
        enviar_respuesta_instruccion(fd, "EXIT");
        return;
    }

    log_info(logger, "PID: %u - Fetch instruccion PC=%u", req->pid, req->pc);

    /* Retardo de respuesta configurado */
    if (memoria_config->retardo_respuesta > 0)
        usleep(memoria_config->retardo_respuesta * 1000);

    char* instruccion = memoria_fetch_instruccion(req->pid, req->pc);
    log_info(logger, "PID: %u - Instruccion: %s", req->pid, instruccion);

    enviar_respuesta_instruccion(fd, instruccion);
    free(instruccion);
    free(req);
}

void memoria_adapter_atender_leer(int fd, t_paquete* paquete)
{
    t_mem_read* req = recibir_lectura_memoria(paquete);
    t_mem_respuesta_lectura resp = { .ok = false, .data = NULL, .size = 0 };

    if (!req) {
        log_error(loggerError, "Read request NULL");
        enviar_respuesta_lectura(fd, &resp);
        return;
    }

    log_info(logger, "PID: %u - Leer dir=%u size=%u", req->pid, req->direccion_logica, req->size);

    void* buffer = malloc(req->size);
    if (!buffer) {
        log_error(loggerError, "PID: %u - malloc failed lectura %u bytes", req->pid, req->size);
        enviar_respuesta_lectura(fd, &resp);
        free(req);
        return;
    }

    if (esquema_leer(req->pid, req->direccion_logica, buffer, req->size)) {
        resp.ok = true;
        resp.data = buffer;
        resp.size = req->size;
    }

    enviar_respuesta_lectura(fd, &resp);
    free(buffer);
    free(req);
}

void memoria_adapter_atender_escribir(int fd, t_paquete* paquete)
{
    t_mem_write* req = recibir_escritura_memoria(paquete);
    if (!req) {
        log_error(loggerError, "Write request NULL");
        enviar_respuesta_fail(fd);
        return;
    }

    log_info(logger, "PID: %u - Escribir dir=%u size=%u", req->pid, req->direccion_logica, req->size);

    if (esquema_escribir(req->pid, req->direccion_logica, req->buffer, req->size)) {
        enviar_respuesta_ok(fd);
    } else {
        enviar_respuesta_fail(fd);
    }

    if (req->buffer) free(req->buffer);
    free(req);
}

void memoria_adapter_atender_resize(int fd, t_paquete* paquete)
{
    uint32_t pid, nuevo_tam;
    paquete_read_uint32(paquete, &pid);
    paquete_read_uint32(paquete, &nuevo_tam);

    log_info(logger, "PID: %u - Resize tam=%u", pid, nuevo_tam);

    if (esquema_resize(pid, nuevo_tam)) {
        log_info(logger, "PID: %u - Resize exitoso", pid);
        enviar_respuesta_ok(fd);
    } else {
        log_error(loggerError, "PID: %u - Resize fallido", pid);
        enviar_respuesta_fail(fd);
    }
}
