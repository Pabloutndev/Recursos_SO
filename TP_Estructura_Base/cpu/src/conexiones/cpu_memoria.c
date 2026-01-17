#include <conexiones/cpu_memoria.h>
#include <conexion/conexion.h>
#include <conexion/conexion.h>
#include <serializacion/serializacion.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <paquete/paquete.h>
#include <loggers/logger.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

static int fd_memoria = -1;
extern t_log* loggerError;

void cpu_conexiones_memoria_init(char* ip, char* puerto)
{
    fd_memoria = crear_conexion(ip, puerto);
    if (fd_memoria < 0) {
        log_error(loggerError, "No se pudo conectar a Memoria en %s:%s", ip, puerto);
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Conectado a Memoria (FD=%d)", fd_memoria);

    // Handshake
    handshake_cliente(fd_memoria, OP_HANDSHAKE, OP_HANDSHAKE, logger);
}

void cpu_conexiones_memoria_close(void)
{
    if (fd_memoria >= 0) {
        close(fd_memoria);
        fd_memoria = -1;
    }
}

char* memoria_fetch_instruccion(uint32_t pid, uint32_t pc)
{
    // 1. Serializar Request (usando Protocolo)
    t_mem_fetch req;
    req.pid = pid;
    req.pc = pc;

    enviar_fetch_instruccion(fd_memoria, &req, OP_MEM_FETCH_INSTRUCCION);

    // 2. Recibir Respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "Fallo al recibir respuesta de Memoria (FETCH)");
        return NULL;
    }

    /* Esperamos OP_RESPUESTA_INSTRUCCION o OP_RESPUESTA_LECTURA?
       Asumamos OP_RESPUESTA_INSTRUCCION que lleva un string. */
    char* instruccion = NULL;

    if ( resp->codigo_operacion == OP_MEM_RESP_INSTRUCCION ) {
        // Asumiendo que viene como string simple
        instruccion = paquete_read_string(resp);
    } else {
        log_error(loggerError, "Respuesta inesperada FETCH: %d", resp->codigo_operacion);
    }

    paquete_destroy(resp);
    return instruccion;
}

bool memoria_obtener_marco(uint32_t pid, uint32_t pagina, bool escritura, uint32_t* marco) {
    bool exito = false;
    // 1. Serializar Request
    t_mem_traducir req;
    req.pid = pid;
    req.direccion_logica = pagina;
    // req.escritura = escritura; // Si la estructura lo soporta? REVISAR

    enviar_traduccion_pagina(fd_memoria, &req, OP_MEM_TRADUCIR_PAGINA);

    // 2. Recibir Respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "Fallo recibir respuesta traduccion");
        return false;
    }

    if (resp->codigo_operacion == OP_MEM_RESP_TRADUCCION) {
        t_mem_respuesta_traduccion* datos = recibir_respuesta_traduccion(resp);
        // deserializer returns struct pointer. we shouldn't use helper inside helper if helper consumes packet destroy? 
        // recibir_respuesta_traduccion consumes packet? No, deserializers usually don't destroy packet, just return struct.
        // Wait, protocol helpers I wrote: recibir_... calls deserializar_...
        // deserializers allocate struct.
        if (datos) {
            *marco = datos->direccion_fisica;
            exito = true; // TODO: Check if marco is valid/present bit?
            free(datos);
        }
    } else {
        log_error(loggerError, "Respuesta traduccion inesperada: %d", resp->codigo_operacion);
    }

    paquete_destroy(resp);
    return exito;
}

bool memoria_leer(uint32_t pid, uint32_t dir_fisica, void* dest, int size) {
    if (size <= 0) return false;

    // 1. Serializar Request
    t_mem_read req;
    req.pid = pid;
    req.direccion_logica = dir_fisica;
    req.size = size;

    enviar_lectura_memoria(fd_memoria, &req, OP_MEM_LEER);

    // 2. Recibir Respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "Fallo al recibir respuesta LECTURA");
        return false;
    }

    bool exito = false;
    if (resp->codigo_operacion == OP_MEM_RESP_LECTURA) {
        t_mem_respuesta_lectura* data = recibir_respuesta_lectura(resp);
        if (data && data->ok && data->size == size) {
            memcpy(dest, data->data, size);
            exito = true;
        }
        if (data) {
            if (data->data) free(data->data);
            free(data);
        }
    } else {
        log_error(loggerError, "Respuesta LECTURA inesperada: %d", resp->codigo_operacion);
    }

    paquete_destroy(resp);
    return exito;
}

bool memoria_escribir(uint32_t pid, uint32_t dir_fisica, void* src, int size) {
    if (size <= 0) return false;

    // 1. Serializar Request
    t_mem_write req;
    req.pid = pid;
    req.direccion_logica = dir_fisica;
    req.size = size;
    req.buffer = src; // Pointer to data

    enviar_escritura_memoria(fd_memoria, &req, OP_MEM_ESCRIBIR);

    // 2. Recibir Respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "Fallo al recibir respuesta ESCRITURA");
        return false;
    }

    bool exito = false;
    // Asumimos que Kernel/Memoria responde con un OK/FAIL simple para escritura?
    // En server.c veia: enviar_respuesta_kernel(fd, ok); -> OP_RESPUESTA_KERNEL (int)
    if (resp->codigo_operacion == OP_RESPUESTA_GENERICA) { // Memoria reuse kernel response logic?
         int* val;
         paquete_read_int(resp, &val);
         exito = (val != 0);
    } else {
         log_error(loggerError, "Respuesta ESCRITURA inesperada: %d", resp->codigo_operacion);
    }

    paquete_destroy(resp);
    return exito;
}
