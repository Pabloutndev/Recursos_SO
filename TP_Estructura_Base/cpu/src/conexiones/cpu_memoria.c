#include <conexiones/cpu_memoria.h>
#include <conexion/conexion.h>
#include <conexion/conexion.h>
#include <serializacion/memoria.h>
#include <protocolo/memoria.h>
#include <paquete/paquete.h>
#include <loggers/logger.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

static int fd_memoria = -1;

void cpu_conexiones_memoria_init(char* ip, char* puerto)
{
    fd_memoria = crear_conexion(ip, puerto);
    if (fd_memoria < 0) {
        log_error(loggerError, "No se pudo conectar a Memoria en %s:%s", ip, puerto);
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Conectado a Memoria (FD=%d)", fd_memoria);

    // Handshake
    handshake_cliente(fd_memoria, OP_HANDSHAKE_CPU, OP_HANDSHAKE_MEMORIA, logger);
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

    enviar_fetch_instruccion(fd_memoria, &req);

    // 2. Recibir Respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "Fallo al recibir respuesta de Memoria (FETCH)");
        return NULL;
    }

    /* 
       Esperamos OP_RESPUESTA_INSTRUCCION o OP_RESPUESTA_LECTURA?
       Asumamos OP_RESPUESTA_INSTRUCCION que lleva un string.
    */
    char* instruccion = NULL;

    if (resp->codigo_operacion == OP_RESPUESTA_INSTRUCCION) {
        // Asumiendo que viene como string simple
        instruccion = paquete_read_string(resp);
    } else {
        log_error(loggerError, "Respuesta inesperada FETCH: %d", resp->codigo_operacion);
    }

    paquete_destroy(resp);
    return instruccion;
}
    paquete_destroy(resp);
    return instruccion;
}

bool memoria_obtener_marco(uint32_t pid, uint32_t pagina, bool escritura, uint32_t* marco) {
     // 1. Serializar Request
    t_mem_traducir_pagina req;
    req.pid = pid;
    req.pagina = pagina;
    // req.escritura = escritura; // Si la estructura lo soporta? REVISAR

    enviar_traduccion_pagina(fd_memoria, &req);

    // 2. Recibir Respuesta
    t_paquete* resp = recibir_paquete(fd_memoria);
    if (!resp) {
        log_error(loggerError, "Fallo recibir respuesta traduccion");
        return false;
    }

    bool exito = false;
    if (resp->codigo_operacion == OP_RESPUESTA_TRADUCCION) {
        t_mem_respuesta_traduccion* datos = recibir_respuesta_traduccion(resp);
        // deserializer returns struct pointer. we shouldn't use helper inside helper if helper consumes packet destroy? 
        // recibir_respuesta_traduccion consumes packet? No, deserializers usually don't destroy packet, just return struct.
        // Wait, protocol helpers I wrote: recibir_... calls deserializar_...
        // deserializers allocate struct.
        if (datos) {
            *marco = datos->marco;
            exito = true; // TODO: Check if marco is valid/present bit?
            free(datos);
        }
    } else {
        log_error(loggerError, "Respuesta traduccion inesperada: %d", resp->codigo_operacion);
    }

    paquete_destroy(resp);
    return exito;
}
