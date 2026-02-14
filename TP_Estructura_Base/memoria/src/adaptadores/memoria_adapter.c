#include "memoria_adapter.h"

#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/string.h>

#include <mod_memoria.h>
#include <configs/memoria_config.h>
#include <gestion/memoria_core.h>
#include <frames/frames.h>
#include <gestion/paginas.h>
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
        log_error(loggerError, "ADAPTER: Init_proceso request NULL");
        enviar_respuesta_fail(fd);
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_INIT_PROCESO - PID=%u PATH=%s", req->pid, req->path);

    /* memoria_crear_proceso carga instrucciones, crea paginacion y escribe a paginas */
    if (!memoria_crear_proceso(req->pid, req->path)) {
        log_error(loggerError, "ADAPTER: Fallo inicializando proceso PID=%u", req->pid);
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
    enviar_respuesta_ok(fd);
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

    if (!pag || !pag->presente) {
        log_error(loggerError, "ADAPTER: Página inválida o error en Page Fault PID=%u PAG=%u", req->pid, pagina);
        enviar_respuesta_traduccion(fd, &resp);
        free(req);
        return;
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

    char* instruccion = memoria_fetch_instruccion(req->pid, req->pc);
    log_info(logger, "ADAPTER: Instrucción enviada: %s", instruccion);

    enviar_respuesta_instruccion(fd, instruccion);
    free(instruccion);
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

    log_info(logger, "ADAPTER: OP_MEM_LEER PID=%u DIR_LOGICA=%u SIZE=%u", req->pid, req->direccion_logica, req->size);

    void* full_buffer = malloc(req->size);
    if (!full_buffer) {
        log_error(loggerError, "ADAPTER: malloc failed para lectura de %u bytes", req->size);
        enviar_respuesta_lectura(fd, &resp);
        free(req);
        return;
    }
    uint32_t bytes_leidos = 0;
    bool error = false;

    while (bytes_leidos < req->size) {
        uint32_t dir_actual = req->direccion_logica + bytes_leidos;
        uint32_t num_pagina = dir_actual / memoria_config->tam_pagina;
        uint32_t offset = dir_actual % memoria_config->tam_pagina;
        uint32_t disponible_en_pagina = memoria_config->tam_pagina - offset;
        uint32_t a_leer = (disponible_en_pagina < (req->size - bytes_leidos)) ? disponible_en_pagina : (req->size - bytes_leidos);

        t_pagina* pag = paginacion_obtener_entrada(req->pid, num_pagina);
        if (!pag || !pag->presente) {
            log_error(loggerError, "ADAPTER: Página %u no presente para PID %u", num_pagina, req->pid);
            error = true;
            break;
        }

        uint32_t dir_fisica = pag->frame * memoria_config->tam_pagina + offset;
        if (!leer_memoria_fisica(dir_fisica, (char*)full_buffer + bytes_leidos, a_leer)) {
            error = true;
            break;
        }
        
        pag->uso = true;
        bytes_leidos += a_leer;
    }

    if (!error) {
        resp.ok = true;
        resp.data = full_buffer;
        resp.size = req->size;
        enviar_respuesta_lectura(fd, &resp);
    } else {
        enviar_respuesta_lectura(fd, &resp);
    }

    if (full_buffer) free(full_buffer);
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

    log_info(logger, "ADAPTER: OP_MEM_ESCRIBIR PID=%u DIR_LOGICA=%u SIZE=%u", req->pid, req->direccion_logica, req->size);

    uint32_t bytes_escritos = 0;
    bool error = false;

    while (bytes_escritos < req->size) {
        uint32_t dir_actual = req->direccion_logica + bytes_escritos;
        uint32_t num_pagina = dir_actual / memoria_config->tam_pagina;
        uint32_t offset = dir_actual % memoria_config->tam_pagina;
        uint32_t disponible_en_pagina = memoria_config->tam_pagina - offset;
        uint32_t a_escribir = (disponible_en_pagina < (req->size - bytes_escritos)) ? disponible_en_pagina : (req->size - bytes_escritos);

        t_pagina* pag = paginacion_obtener_entrada(req->pid, num_pagina);
        if (!pag || !pag->presente) {
            log_error(loggerError, "ADAPTER: Página %u no presente para PID %u", num_pagina, req->pid);
            error = true;
            break;
        }

        uint32_t dir_fisica = pag->frame * memoria_config->tam_pagina + offset;
        if (!escribir_memoria_fisica(dir_fisica, (char*)req->buffer + bytes_escritos, a_escribir)) {
            error = true;
            break;
        }
        
        pag->uso = true;
        pag->modificado = true;
        bytes_escritos += a_escribir;
    }

    if (!error) {
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

    log_info(logger, "ADAPTER: OP_MEM_RESIZE PID=%u NUEVO_TAM=%u", pid, nuevo_tam);

    if (paginacion_resize(pid, nuevo_tam)) {
        log_info(logger, "ADAPTER: Resize PID %u EXITOSO", pid);
        enviar_respuesta_ok(fd);
    } else {
        log_error(loggerError, "ADAPTER: Resize PID %u FALLIDO (Sin espacio o error interno)", pid);
        enviar_respuesta_fail(fd);
    }
}
