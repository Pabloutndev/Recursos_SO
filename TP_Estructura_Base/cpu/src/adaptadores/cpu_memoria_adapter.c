#include "cpu_memoria_adapter.h"
#include <model/model.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <commons/log.h>

static int fd_memoria = -1;
extern t_log* logger;
extern t_log* loggerError;

/**
 * NOTA: Inicializar con cpu_memoria_adapter_init() desde main.c
 */

int cpu_memoria_adapter_init(int fd_mem) {
    fd_memoria = fd_mem;
    if (fd_memoria < 0) {
        log_error(loggerError, "Socket memoria invalido");
        return -1;
    }
    return 0;
}

/* ========================================
 * TRANSFORMACIÓN DE ESTRUCTURAS
 * ======================================== */

t_mem_fetch* cpu_a_mem_fetch(uint32_t pid, uint32_t pc)
{
    t_mem_fetch* req = malloc(sizeof(t_mem_fetch));
    req->pid = pid;
    req->pc = pc;
    return req;
}

t_mem_traducir* cpu_a_mem_traduccion(uint32_t pid, uint32_t pagina)
{
    t_mem_traducir* req = malloc(sizeof(t_mem_traducir));
    req->pid = pid;
    req->direccion_logica = pagina;
    return req;
}

t_mem_read* cpu_a_mem_read(uint32_t pid, uint32_t direccion, uint32_t size)
{
    t_mem_read* req = malloc(sizeof(t_mem_read));
    req->pid = pid;
    req->direccion_logica = direccion;
    req->size = size;
    return req;
}

t_mem_write* cpu_a_mem_write(uint32_t pid, uint32_t direccion, 
                              void* buffer, uint32_t size)
{
    t_mem_write* req = malloc(sizeof(t_mem_write));
    req->pid = pid;
    req->direccion_logica = direccion;
    req->size = size;
    req->buffer = buffer;
    return req;
}

/* ========================================
 * OPERACIONES COMPLETAS (REQUEST + RESPONSE)
 * ======================================== */

char* cpu_fetch_instruccion(uint32_t pid, uint32_t pc)
{
    // Paso 1: Preparar request
    t_mem_fetch* req = cpu_a_mem_fetch(pid, pc);

    // Paso 2: Enviar a Memoria usando protocolo
    log_info(logger, "PID: %u - Fetch instruccion PC=%u",
             pid, pc);
    enviar_fetch_instruccion(fd_memoria, req, OP_MEM_FETCH_INSTRUCCION);

    // Paso 3: Recibir respuesta (paquete con instrucción)
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "PID: %u - Error recibiendo respuesta fetch", pid);
        free(req);
        return NULL;
    }

    char* instruccion = NULL;

    // Paso 4: Validar op_code y deserializar
    if (resp->codigo_operacion == OP_MEM_RESP_INSTRUCCION) {
        // Deserializar string (instrucción)
        instruccion = paquete_read_string(resp);
        if (!instruccion) {
            instruccion = strdup("EXIT");  // Fallback
            log_warning(logger, "PID: %u - Instruccion vacia de memoria", pid);
        }
        log_info(logger, "PID: %u - Fetch OK instruccion: %s", pid, instruccion);
    } else {
        log_error(loggerError, "PID: %u - Respuesta fetch inesperada: %d",
                 pid, resp->codigo_operacion);
        instruccion = strdup("EXIT");  // Fallback
    }

    paquete_destroy(resp);
    free(req);
    return instruccion;
}

bool cpu_traducir_pagina(uint32_t pid, uint32_t pagina, 
                                         uint32_t* marco)
{
    if (!marco) {
        log_error(loggerError, "PID: %u - Marco NULL en traduccion", pid);
        return false;
    }

    // Paso 1: Preparar request
    t_mem_traducir* req = cpu_a_mem_traduccion(pid, pagina);

    // Paso 2: Enviar a Memoria
    log_info(logger, "PID: %u - Traducir pagina=%u",
             pid, pagina);
    enviar_traduccion_pagina(fd_memoria, req, OP_MEM_TRADUCIR_PAGINA);

    // Paso 3: Recibir respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "PID: %u - Error recibiendo respuesta traduccion", pid);
        free(req);
        return false;
    }

    bool exito = false;

    // Paso 4: Procesar respuesta
    if (resp->codigo_operacion == OP_MEM_RESP_TRADUCCION) {
        t_mem_respuesta_traduccion* datos = recibir_respuesta_traduccion(resp);
        if (datos && datos->ok) {
            *marco = datos->direccion_fisica;
            exito = true;
            log_info(logger, "PID: %u - Traduccion OK marco=%u", pid, *marco);
        } else {
            log_warning(logger, "PID: %u - Traduccion fallida (page fault?)", pid);
        }
        if (datos) free(datos);
    } else {
        log_error(loggerError, "PID: %u - Respuesta traduccion inesperada: %d",
                 pid, resp->codigo_operacion);
    }

    paquete_destroy(resp);
    free(req);
    return exito;
}

bool cpu_leer(uint32_t pid, uint32_t dir_fisica, 
                               void* buffer, uint32_t size)
{
    if (!buffer || size == 0) {
        log_error(loggerError, "PID: %u - Parametros invalidos en lectura", pid);
        return false;
    }

    // Paso 1: Preparar request
    t_mem_read* req = cpu_a_mem_read(pid, dir_fisica, size);

    // Paso 2: Enviar a Memoria
    log_info(logger, "PID: %u - Lectura memoria dir=%u tam=%u",
             pid, dir_fisica, size);
    enviar_lectura_memoria(fd_memoria, req, OP_MEM_LEER);

    // Paso 3: Recibir respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "PID: %u - Error recibiendo respuesta lectura", pid);
        free(req);
        return false;
    }

    bool exito = false;

    // Paso 4: Procesar respuesta
    if (resp->codigo_operacion == OP_MEM_RESP_LECTURA) {
        t_mem_respuesta_lectura* data = recibir_respuesta_lectura(resp);
        if (data && data->ok && data->size == size && data->data) {
            memcpy(buffer, data->data, size);
            exito = true;
            log_info(logger, "PID: %u - Lectura OK %u bytes", pid, size);
        } else {
            log_error(loggerError, "PID: %u - Lectura fallida o tamanio inconsistente", pid);
        }
        if (data) {
            if (data->data) free(data->data);
            free(data);
        }
    } else {
        log_error(loggerError, "PID: %u - Respuesta lectura inesperada: %d",
                 pid, resp->codigo_operacion);
    }

    paquete_destroy(resp);
    free(req);
    return exito;
}

bool cpu_escribir(uint32_t pid, uint32_t dir_fisica, 
                                   void* buffer, uint32_t size)
{
    if (!buffer || size == 0) {
        log_error(loggerError, "PID: %u - Parametros invalidos en escritura", pid);
        return false;
    }

    // Paso 1: Preparar request (NO copiamos buffer, solo pasamos puntero)
    // IMPORTANTE: enviar_escritura_memoria NO debe liberar buffer
    t_mem_write req = {
        .pid = pid,
        .direccion_logica = dir_fisica,
        .size = size,
        .buffer = buffer
    };

    // Paso 2: Enviar a Memoria
    log_info(logger, "PID: %u - Escritura memoria dir=%u tam=%u",
             pid, dir_fisica, size);
    enviar_escritura_memoria(fd_memoria, &req, OP_MEM_ESCRIBIR);

    // Paso 3: Recibir respuesta (paquete con OP_OK o OP_FAIL)
    t_paquete* resp = recibir_paquete(fd_memoria);

    if (!resp) {
        log_error(loggerError, "PID: %u - Error recibiendo respuesta escritura", pid);
        return false;
    }

    bool exito = recibir_respuesta(resp);
    
    if (exito) {
        log_info(logger, "PID: %u - Escritura OK %u bytes", pid, size);
    } else {
        log_error(loggerError, "PID: %u - Escritura fallida dir=%u", pid, dir_fisica);
    }

    paquete_destroy(resp);
    return exito;
}
