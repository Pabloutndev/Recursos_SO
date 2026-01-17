#include "memoria_adapter.h"

#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/string.h>

#include <mod_memoria.h>
#include <configs/memoria_config.h>
#include <gestion/memoria_core.h>
#include <frames/frames.h>
#include <gestion/paginas.h>
#include <model/model.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>

#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

extern t_log* logger;
extern t_log* loggerError;
extern t_memoria_config* memoria_config;

/* ========================================
 * REQUEST HANDLERS (ADAPTADORES)
 * ======================================== */

void memoria_adapter_init_proceso(t_mem_init_proceso* req, int socket_kernel)
{
    if (!req) {
        log_error(loggerError, "ADAPTER: Init_proceso request NULL");
        enviar_respuesta_fail(socket_kernel);
        return;
    }

    log_info(logger,
             "ADAPTER: OP_MEM_INIT_PROCESO - PID=%u TAM=%u PATH=%s",
             req->pid, req->tamanio, req->path);

    /* 1. Crear estructuras de paginación */
    if (!paginacion_crear_proceso(req->pid, req->tamanio)) {
        log_error(loggerError,
                  "ADAPTER: Fallo creando paginación PID=%u", req->pid);
        enviar_respuesta_fail(socket_kernel);
        return;
    }

    /* 2. Cargar instrucciones en memoria lógica */
    if (!memoria_crear_proceso(req->pid, req->path)) {
        log_error(loggerError,
                  "ADAPTER: Fallo cargando instrucciones PID=%u", req->pid);

        paginacion_destruir_proceso(req->pid);
        enviar_respuesta_fail(socket_kernel);
        return;
    }

    log_info(logger,
             "ADAPTER: Proceso %u inicializado correctamente", req->pid);

    enviar_respuesta_ok(socket_kernel);
}

void memoria_adapter_fin_proceso(t_mem_fin_proceso* req, int socket_kernel)
{
    if (!req) {
        log_error(loggerError, "ADAPTER: Fin_proceso request NULL");
        return;
    }

    log_info(logger,
             "ADAPTER: OP_MEM_FIN_PROCESO - PID=%u",
             req->pid);

    /* Liberar estructuras de memoria */
    paginacion_destruir_proceso(req->pid);
    memoria_destruir_proceso(req->pid);

    log_info(logger,
             "ADAPTER: Proceso %u eliminado de Memoria",
             req->pid);
}

void memoria_adapter_traducir_pagina(t_mem_traducir* req, int socket_cpu)
{
    t_mem_respuesta_traduccion resp = {
        .ok = false,
        .direccion_fisica = 0
    };

    if (!req) {
        log_error(loggerError, "ADAPTER: Traducir request NULL");
        enviar_respuesta_traduccion(socket_cpu, &resp);
        return;
    }

    uint32_t pagina  = req->direccion_logica / memoria_config->tam_pagina;

    log_info(logger,
             "ADAPTER: OP_MEM_TRADUCIR_PAGINA PID=%u PAG=%u",
             req->pid, pagina);

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag) {
        log_error(loggerError,
                  "ADAPTER: Página inválida PID=%u PAG=%u",
                  req->pid, pagina);
        enviar_respuesta_traduccion(socket_cpu, &resp);
        return;
    }

    /* Resolver page fault si es necesario */
    if (!pag->presente) {
        int frame = obtener_frame_libre();

        if (frame < 0) {
            log_error(loggerError,
                      "ADAPTER: Sin frames libres PID=%u",
                      req->pid);
            enviar_respuesta_traduccion(socket_cpu, &resp);
            return;
        }

        pag->frame      = frame;
        pag->presente   = true;
        pag->uso        = true;
        pag->modificado = false;

        log_info(logger,
                 "ADAPTER: Page fault resuelto PID=%u FRAME=%d",
                 req->pid, frame);
    }

    /* Dirección física real */
    resp.ok = true;
    resp.direccion_fisica =
        pag->frame * memoria_config->tam_pagina;

    enviar_respuesta_traduccion(socket_cpu, &resp);
}

void memoria_adapter_fetch_instruccion(t_mem_fetch* req, int socket_cpu) {
    if (!req) {
        log_error(loggerError, "ADAPTER: Fetch request NULL");
        enviar_respuesta_instruccion(socket_cpu, "EXIT");
        return;
    }

    log_info(logger,
             "ADAPTER: OP_MEM_FETCH_INSTRUCCION PID=%u PC=%u",
             req->pid, req->pc);

    const char* instruccion =
        memoria_fetch_instruccion(req->pid, req->pc);

    log_info(logger,
             "ADAPTER: Instrucción enviada: %s",
             instruccion);

    enviar_respuesta_instruccion(socket_cpu, instruccion);
}

void memoria_adapter_leer(t_mem_read* req, int socket_cpu)
{
   t_mem_respuesta_lectura resp = {
        .ok   = false,
        .data = NULL,
        .size = 0
    };

    if (!req) {
        log_error(loggerError, "ADAPTER: Read request NULL");
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    uint32_t pagina = req->direccion_logica / memoria_config->tam_pagina;
    uint32_t offset = req->direccion_logica % memoria_config->tam_pagina;

    if (offset + req->size > memoria_config->tam_pagina) {
        log_error(loggerError,
                  "ADAPTER: Lectura cruza pagina PID=%u",
                  req->pid);
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag || !pag->presente) {
        log_error(loggerError,
                  "ADAPTER: Página no presente PID=%u PAG=%u",
                  req->pid, pagina);
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    uint32_t dir_fisica =
        pag->frame * memoria_config->tam_pagina + offset;

    void* buffer = malloc(req->size);
    if (!buffer) {
        log_error(loggerError, "ADAPTER: malloc fallido");
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    if (!leer_memoria_fisica(dir_fisica, buffer, req->size)) {
        free(buffer);
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    pag->uso = true;

    resp.ok   = true;
    resp.data = buffer;
    resp.size = req->size;

    enviar_respuesta_lectura(socket_cpu, &resp);

    free(buffer);
}

void memoria_adapter_escribir(t_mem_write* req, int socket_cpu)
{
    if (!req) {
        log_error(loggerError, "ADAPTER: Write request NULL");
        enviar_respuesta_fail(socket_cpu);
        return;
    }

    uint32_t pagina = req->direccion_logica / memoria_config->tam_pagina;
    uint32_t offset = req->direccion_logica % memoria_config->tam_pagina;

    if (offset + req->size > memoria_config->tam_pagina) {
        log_error(loggerError,
                  "ADAPTER: Escritura cruza pagina PID=%u",
                  req->pid);
        enviar_respuesta_fail(socket_cpu);
        return;
    }

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag || !pag->presente) {
        log_error(loggerError,
                  "ADAPTER: Página no presente PID=%u PAG=%u",
                  req->pid, pagina);
        enviar_respuesta_fail(socket_cpu);
        return;
    }

    uint32_t dir_fisica =
        pag->frame * memoria_config->tam_pagina + offset;

    if (!escribir_memoria_fisica(dir_fisica, req->buffer, req->size)) {
        enviar_respuesta_fail(socket_cpu);
        return;
    }

    pag->uso        = true;
    pag->modificado = true;

    enviar_respuesta_ok(socket_cpu);
}
