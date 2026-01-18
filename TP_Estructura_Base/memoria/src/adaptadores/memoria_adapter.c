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
 * REQUEST HANDLERS (ADAPTADORES) - ATENCION DE MENSAJES
 * ======================================== */

void memoria_adapter_atender_init_proceso(int fd, t_paquete* paquete)
{
    t_mem_init_proceso* req = recibir_init_proceso(paquete);
    if (!req) {
        log_error(loggerError, "ADAPTER: Init_proceso request NULL");
        enviar_respuesta_fail(fd);
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_INIT_PROCESO - PID=%u PATH=%s", req->pid, req->path);

    /* 1. Crear estructuras de paginación (suponemos tamaño viene de config o está en struct) 
     * Nota: Si el TP requiere un tamaño específico, debería estar en t_mem_init_proceso.
     * Por ahora usamos un valor por defecto o el que venga en el struct si lo actualizamos. */
    uint32_t tamanio = req->tamanio; 

    if (!paginacion_crear_proceso(req->pid, tamanio)) {
        log_error(loggerError, "ADAPTER: Fallo creando paginación PID=%u", req->pid);
        enviar_respuesta_fail(fd);
        free(req);
        return;
    }

    /* 2. Cargar instrucciones en memoria lógica */
    if (!memoria_crear_proceso(req->pid, req->path)) {
        log_error(loggerError, "ADAPTER: Fallo cargando instrucciones PID=%u", req->pid);
        paginacion_destruir_proceso(req->pid);
        enviar_respuesta_fail(fd);
        free(req);
        return;
    }

    log_info(logger, "ADAPTER: Proceso %u inicializado correctamente", req->pid);
    enviar_respuesta_ok(fd);
    free(req);
}

void memoria_adapter_atender_fin_proceso(int fd, t_paquete* paquete)
{
    t_mem_fin_proceso* req = recibir_fin_proceso(paquete);
    if (!req) {
        log_error(loggerError, "ADAPTER: Fin_proceso request NULL");
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_FIN_PROCESO - PID=%u", req->pid);

    /* Liberar estructuras de memoria */
    paginacion_destruir_proceso(req->pid);
    memoria_destruir_proceso(req->pid);

    log_info(logger, "ADAPTER: Proceso %u eliminado de Memoria", req->pid);
    // Normalmente no requiere respuesta pero si el protocolo lo pide:
    // enviar_respuesta_ok(fd);
    free(req);
}

void memoria_adapter_atender_traducir_pagina(int fd, t_paquete* paquete)
{
    t_mem_traducir* req = recibir_mem_traducir_pagina(paquete);
    t_mem_respuesta_traduccion resp = { .ok = false, .direccion_fisica = 0 };

    if (!req) {
        log_error(loggerError, "ADAPTER: Traducir request NULL");
        enviar_respuesta_traduccion(fd, &resp);
        return;
    }

    uint32_t pagina = req->direccion_logica / memoria_config->tam_pagina;
    log_info(logger, "ADAPTER: OP_MEM_TRADUCIR_PAGINA PID=%u PAG=%u", req->pid, pagina);

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag) {
        log_error(loggerError, "ADAPTER: Página inválida PID=%u PAG=%u", req->pid, pagina);
        enviar_respuesta_traduccion(fd, &resp);
        free(req);
        return;
    }

    /* Resolver page fault si es necesario */
    if (!pag->presente) {
        int frame = obtener_frame_libre();
        if (frame < 0) {
            log_error(loggerError, "ADAPTER: Sin frames libres PID=%u", req->pid);
            enviar_respuesta_traduccion(fd, &resp);
            free(req);
            return;
        }
        pag->frame = frame;
        pag->presente = true;
        pag->uso = true;
        pag->modificado = false;
        log_info(logger, "ADAPTER: Page fault resuelto PID=%u FRAME=%d", req->pid, frame);
    }

    resp.ok = true;
    resp.direccion_fisica = pag->frame * memoria_config->tam_pagina;

    enviar_respuesta_traduccion(fd, &resp);
    free(req);
}

void memoria_adapter_atender_fetch_instruccion(int fd, t_paquete* paquete) {
    t_mem_fetch* req = recibir_fetch(paquete);
    if (!req) {
        log_error(loggerError, "ADAPTER: Fetch request NULL");
        enviar_respuesta_instruccion(fd, "EXIT");
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_FETCH_INSTRUCCION PID=%u PC=%u", req->pid, req->pc);

    /* Retardo de respuesta configurado */
    if (memoria_config->retardo_respuesta > 0)
        usleep(memoria_config->retardo_respuesta * 1000);

    const char* instruccion = memoria_fetch_instruccion(req->pid, req->pc);
    log_info(logger, "ADAPTER: Instrucción enviada: %s", instruccion);

    enviar_respuesta_instruccion(fd, (char*)instruccion);
    free(req);
}

void memoria_adapter_atender_leer(int fd, t_paquete* paquete)
{
    t_mem_read* req = recibir_lectura_memoria(paquete);
    t_mem_respuesta_lectura resp = { .ok = false, .data = NULL, .size = 0 };

    if (!req) {
        log_error(loggerError, "ADAPTER: Read request NULL");
        enviar_respuesta_lectura(fd, &resp);
        return;
    }

    uint32_t pagina = req->direccion_logica / memoria_config->tam_pagina;
    uint32_t offset = req->direccion_logica % memoria_config->tam_pagina;

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag || !pag->presente) {
        log_error(loggerError, "ADAPTER: Página no presente PID=%u PAG=%u", req->pid, pagina);
        enviar_respuesta_lectura(fd, &resp);
        free(req);
        return;
    }

    uint32_t dir_fisica = pag->frame * memoria_config->tam_pagina + offset;
    void* buffer = malloc(req->size);

    if (buffer && leer_memoria_fisica(dir_fisica, buffer, req->size)) {
        pag->uso = true;
        resp.ok = true;
        resp.data = buffer;
        resp.size = req->size;
        enviar_respuesta_lectura(fd, &resp);
    } else {
        enviar_respuesta_lectura(fd, &resp);
    }

    if (buffer) free(buffer);
    free(req);
}

void memoria_adapter_atender_escribir(int fd, t_paquete* paquete)
{
    t_mem_write* req = recibir_escritura_memoria(paquete);
    if (!req) {
        log_error(loggerError, "ADAPTER: Write request NULL");
        enviar_respuesta_fail(fd);
        return;
    }

    uint32_t pagina = req->direccion_logica / memoria_config->tam_pagina;
    uint32_t offset = req->direccion_logica % memoria_config->tam_pagina;

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag || !pag->presente) {
        log_error(loggerError, "ADAPTER: Página no presente PID=%u PAG=%u", req->pid, pagina);
        enviar_respuesta_fail(fd);
        free(req);
        return;
    }

    uint32_t dir_fisica = pag->frame * memoria_config->tam_pagina + offset;

    if (escribir_memoria_fisica(dir_fisica, req->buffer, req->size)) {
        pag->uso = true;
        pag->modificado = true;
        enviar_respuesta_ok(fd);
    } else {
        enviar_respuesta_fail(fd);
    }

    if (req->buffer) free(req->buffer);
    free(req);
}
