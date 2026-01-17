#include <conexiones/cpu_memoria.h>
#include <adaptadores/cpu_memoria_adapter.h>
#include <conexion/conexion.h>
#include <loggers/logger.h>
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
    
    // Inicializar adapter con el file descriptor
    cpu_memoria_adapter_init(fd_memoria);
}

void cpu_conexiones_memoria_close(void)
{
    if (fd_memoria >= 0) {
        close(fd_memoria);
        fd_memoria = -1;
    }
}

/* WRAPPERS PARA COMPATIBILIDAD - Delegación a Adaptadores */

char* memoria_fetch_instruccion(uint32_t pid, uint32_t pc)
{
    return cpu_fetch_instruccion(pid, pc);
}

bool memoria_obtener_marco(uint32_t pid, uint32_t pagina, bool escritura, uint32_t* marco)
{
    // Nota: parámetro 'escritura' no se usa (la traducción es independiente del acceso)
    return cpu_traducir_pagina(pid, pagina, marco);
}

bool memoria_leer(uint32_t pid, uint32_t dir_fisica, void* dest, int size)
{
    return cpu_leer(pid, dir_fisica, dest, (uint32_t)size);
}

bool memoria_escribir(uint32_t pid, uint32_t dir_fisica, void* src, int size)
{
    return cpu_escribir(pid, dir_fisica, src, (uint32_t)size);
}
