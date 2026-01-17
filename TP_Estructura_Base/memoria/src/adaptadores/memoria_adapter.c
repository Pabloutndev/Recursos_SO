#include "memoria_adapter.h"

#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/string.h>

#include <mod_memoria.h>
#include <configs/memoria_config.h>
#include <gestion/memoria_core.h>
#include <frames/frames.h>
#include <gestion/paginas.h>
#include <common/memoria/memoria.h>
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

    log_info(logger, "ADAPTER: OP_MEM_INIT_PROCESO - PID=%u, TAM=%u",
             req->pid, req->tamanio);

    // Lógica interna
    bool ok = paginacion_crear_proceso(req->pid, req->tamanio);

    if (ok) {
        log_info(logger, "ADAPTER: Proceso %u iniciado en Memoria", req->pid);
        enviar_respuesta_ok(socket_kernel);
    } else {
        log_warning(logger, "ADAPTER: Fallo creando proceso %u (¿ya existe?)", req->pid);
        enviar_respuesta_fail(socket_kernel);
    }
}

void memoria_adapter_fin_proceso(t_mem_fin_proceso* req, int socket_kernel)
{
    if (!req) {
        log_error(loggerError, "ADAPTER: Fin_proceso request NULL");
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_FIN_PROCESO - PID=%u", req->pid);

    // Lógica interna (one-way, no responde)
    paginacion_destruir_proceso(req->pid);
    
    log_info(logger, "ADAPTER: Proceso %u finalizado en Memoria", req->pid);
}

void memoria_adapter_traducir_pagina(t_mem_traducir* req, int socket_cpu)
{
    if (!req) {
        log_error(loggerError, "ADAPTER: Traducir request NULL");
        // Enviar respuesta de error
        t_mem_respuesta_traduccion resp = { .ok = false, .direccion_fisica = 0 };
        enviar_respuesta_traduccion(socket_cpu, &resp);
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_TRADUCIR_PAGINA - PID=%u, PAGINA=%u",
             req->pid, req->direccion_logica);

    t_mem_respuesta_traduccion resp = { .ok = false, .direccion_fisica = 0 };

    // Obtener entrada en tabla de páginas
    t_pagina* pag = paginacion_obtener_entrada(req->pid, req->direccion_logica);

    if (!pag) {
        log_error(loggerError, "ADAPTER: Página inválida PID=%u PAG=%u",
                 req->pid, req->direccion_logica);
        enviar_respuesta_traduccion(socket_cpu, &resp);
        return;
    }

    // Si no está presente (page fault), asignar frame
    if (!pag->presente) {
        int frame = obtener_frame_libre();
        
        if (frame < 0) {
            log_error(loggerError, "ADAPTER: Sin frames libres (PID=%u)",
                     req->pid);
            enviar_respuesta_traduccion(socket_cpu, &resp);
            return;
        }

        pag->frame = frame;
        pag->presente = true;
        pag->uso = true;
        pag->modificado = false;
        log_info(logger, "ADAPTER: Page fault resuelto - Marco %d asignado", frame);
    }

    // Respuesta exitosa
    resp.ok = true;
    resp.direccion_fisica = pag->frame;

    log_info(logger, "ADAPTER: Traducción OK - PID=%u, PAGINA=%u -> MARCO=%u",
             req->pid, req->direccion_logica, pag->frame);
    
    enviar_respuesta_traduccion(socket_cpu, &resp);
}

void memoria_adapter_fetch_instruccion(t_mem_fetch* req, int socket_cpu)
{
    if (!req) {
        log_error(loggerError, "ADAPTER: Fetch request NULL");
        enviar_respuesta_instruccion(socket_cpu, "EXIT");
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_FETCH_INSTRUCCION - PID=%u, PC=%u",
             req->pid, req->pc);

    // Simular retardo (si está configurado)
    if (memoria_config->retardo_respuesta > 0) {
        usleep(memoria_config->retardo_respuesta * 1000);
    }

    // Lógica interna: leer instrucción desde dirección PC
    char* instruccion = paginacion_leer_instruccion(req->pid, req->pc);

    if (!instruccion) {
        log_error(loggerError, "ADAPTER: Error leyendo instrucción PID=%u PC=%u",
                 req->pid, req->pc);
        instruccion = strdup("EXIT");  // Fallback
    }

    log_info(logger, "ADAPTER: Fetch enviando instrucción: %s", instruccion);
    
    // Enviar respuesta
    enviar_respuesta_instruccion(socket_cpu, instruccion);
    
    free(instruccion);
}

void memoria_adapter_leer(t_mem_read* req, int socket_cpu)
{
    if (!req) {
        log_error(loggerError, "ADAPTER: Read request NULL");
        t_mem_respuesta_lectura resp = { .ok = false, .data = NULL, .size = 0 };
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    log_info(logger, "ADAPTER: OP_MEM_LEER - PID=%u, DIR=%u, TAM=%u",
             req->pid, req->direccion_logica, req->size);

    t_mem_respuesta_lectura resp = {
        .ok = false,
        .data = NULL,
        .size = req->size
    };

    // Traducir dirección lógica a física
    int pagina = req->direccion_logica / memoria_config->tam_pagina;
    int offset = req->direccion_logica % memoria_config->tam_pagina;

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag || !pag->presente) {
        log_error(loggerError, "ADAPTER: Página no presente PID=%u PAG=%u",
                 req->pid, pagina);
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    // Calcular dirección física
    uint32_t dir_fisica = pag->frame * memoria_config->tam_pagina + offset;

    // Leer desde memoria
    void* buffer = malloc(req->size);
    if (!buffer) {
        log_error(loggerError, "ADAPTER: Fallo malloc en lectura");
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    if (!leer_memoria_fisica(dir_fisica, buffer, req->size)) {
        log_error(loggerError, "ADAPTER: Fallo leyendo memoria física");
        free(buffer);
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    // Respuesta exitosa
    resp.ok = true;
    resp.data = buffer;

    log_info(logger, "ADAPTER: Lectura OK - %u bytes", req->size);
    
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

    log_info(logger, "ADAPTER: OP_MEM_ESCRIBIR - PID=%u, DIR=%u, TAM=%u",
             req->pid, req->direccion_logica, req->size);

    // Traducir dirección lógica a física
    int pagina = req->direccion_logica / memoria_config->tam_pagina;
    int offset = req->direccion_logica % memoria_config->tam_pagina;

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag || !pag->presente) {
        log_error(loggerError, "ADAPTER: Página no presente PID=%u PAG=%u",
                 req->pid, pagina);
        enviar_respuesta_fail(socket_cpu);
        return;
    }

    // Calcular dirección física
    uint32_t dir_fisica = pag->frame * memoria_config->tam_pagina + offset;

    // Escribir en memoria
    if (!escribir_memoria_fisica(dir_fisica, req->buffer, req->size)) {
        log_error(loggerError, "ADAPTER: Fallo escribiendo memoria física");
        enviar_respuesta_fail(socket_cpu);
        return;
    }

    // Marcar página como modificada
    pag->modificado = true;
    pag->uso = true;

    log_info(logger, "ADAPTER: Escritura OK - %u bytes", req->size);
    
    enviar_respuesta_ok(socket_cpu);
}
