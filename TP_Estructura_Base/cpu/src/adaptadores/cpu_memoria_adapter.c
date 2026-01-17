#include "cpu_memoria_adapter.h"
#include <common/memoria/memoria.h>
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
        log_error(loggerError, "ADAPTER: Socket Memoria inválido");
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
    log_info(logger, "ADAPTER: Enviando OP_MEM_FETCH_INSTRUCCION (PID=%u, PC=%u)",
             pid, pc);
    enviar_fetch_instruccion(fd_memoria, req, OP_MEM_FETCH_INSTRUCCION);

    // Paso 3: Recibir respuesta (paquete con instrucción)
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "ADAPTER: Error recibiendo respuesta FETCH");
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
            log_warning(logger, "ADAPTER: Instrucción vacía de Memoria");
        }
        log_info(logger, "ADAPTER: Fetch OK - Instrucción: %s", instruccion);
    } else {
        log_error(loggerError, "ADAPTER: Respuesta FETCH inesperada: %d",
                 resp->codigo_operacion);
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
        log_error(loggerError, "ADAPTER: Marco NULL en traducción");
        return false;
    }

    // Paso 1: Preparar request
    t_mem_traducir* req = cpu_a_mem_traduccion(pid, pagina);

    // Paso 2: Enviar a Memoria
    log_info(logger, "ADAPTER: Enviando OP_MEM_TRADUCIR_PAGINA (PID=%u, PAG=%u)",
             pid, pagina);
    enviar_traduccion_pagina(fd_memoria, req, OP_MEM_TRADUCIR_PAGINA);

    // Paso 3: Recibir respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "ADAPTER: Error recibiendo respuesta traducción");
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
            log_info(logger, "ADAPTER: Traducción OK - Marco: %u", *marco);
        } else {
            log_warning(logger, "ADAPTER: Traducción fallida (page fault?)");
        }
        if (datos) free(datos);
    } else {
        log_error(loggerError, "ADAPTER: Respuesta traducción inesperada: %d",
                 resp->codigo_operacion);
    }

    paquete_destroy(resp);
    free(req);
    return exito;
}

bool cpu_leer(uint32_t pid, uint32_t dir_fisica, 
                               void* buffer, uint32_t size)
{
    if (!buffer || size == 0) {
        log_error(loggerError, "ADAPTER: Parámetros inválidos en lectura");
        return false;
    }

    // Paso 1: Preparar request
    t_mem_read* req = cpu_a_mem_read(pid, dir_fisica, size);

    // Paso 2: Enviar a Memoria
    log_info(logger, "ADAPTER: Enviando OP_MEM_LEER (PID=%u, DIR=%u, TAM=%u)",
             pid, dir_fisica, size);
    enviar_lectura_memoria(fd_memoria, req, OP_MEM_LEER);

    // Paso 3: Recibir respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "ADAPTER: Error recibiendo respuesta lectura");
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
            log_info(logger, "ADAPTER: Lectura OK - %u bytes", size);
        } else {
            log_error(loggerError, "ADAPTER: Lectura fallida o tamaño inconsistente");
        }
        if (data) {
            if (data->data) free(data->data);
            free(data);
        }
    } else {
        log_error(loggerError, "ADAPTER: Respuesta lectura inesperada: %d",
                 resp->codigo_operacion);
    }

    paquete_destroy(resp);
    free(req);
    return exito;
}

bool cpu_escribir(uint32_t pid, uint32_t dir_fisica, 
                                   void* buffer, uint32_t size)
{
    if (!buffer || size == 0) {
        log_error(loggerError, "ADAPTER: Parámetros inválidos en escritura");
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
    log_info(logger, "ADAPTER: Enviando OP_MEM_ESCRIBIR (PID=%u, DIR=%u, TAM=%u)",
             pid, dir_fisica, size);
    enviar_escritura_memoria(fd_memoria, &req, OP_MEM_ESCRIBIR);

    // Paso 3: Recibir respuesta (int: 1=OK, 0=FAIL)
    int respuesta = 0;
    int bytes_recibidos = recv(fd_memoria, &respuesta, sizeof(int), MSG_WAITALL);

    bool exito = (bytes_recibidos > 0 && respuesta == 1);
    
    if (exito) {
        log_info(logger, "ADAPTER: Escritura OK");
    } else {
        log_error(loggerError, "ADAPTER: Escritura fallida");
    }

    return exito;
}
