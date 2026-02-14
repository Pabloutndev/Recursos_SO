#include <gestion/memoria_core.h>
#include <gestion/memoria_ram.h>
#include <frames/frames.h>
#include <configs/memoria_config.h>
#include <swap/swap.h>
#include <server/server_mem.h>
#include <unistd.h>
#include <commons/log.h>
#include <stdlib.h>
#include "mod_memoria.h"

/// Contexto Global del Modulo
t_memoria_context MEMORIA_CTX;

// Globals usados por extern en sub-modulos
t_log* logger = NULL;
t_log* loggerError = NULL;
t_memoria_config* memoria_config = NULL;

int memoria_init(const char* path_config) {
    MEMORIA_CTX.logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
    MEMORIA_CTX.logger_error = log_create("memoria_error.log", "MEMORIA_ERR", 1, LOG_LEVEL_ERROR);

    if (!MEMORIA_CTX.logger || !MEMORIA_CTX.logger_error) {
        return EXIT_FAILURE;
    }

    // Setear globals para extern en sub-modulos
    logger = MEMORIA_CTX.logger;
    loggerError = MEMORIA_CTX.logger_error;

    log_info(MEMORIA_CTX.logger, "Iniciando modulo Memoria...");

    MEMORIA_CTX.config = memoria_cargar_config(path_config);
    if (!MEMORIA_CTX.config) {
        log_error(MEMORIA_CTX.logger, "Error al cargar configuración");
        return EXIT_FAILURE;
    }

    memoria_config = MEMORIA_CTX.config;

    // 1. Iniciar RAM (User Space)
    if (memoria_ram_init() != 0) {
        log_error(MEMORIA_CTX.logger, "Error al iniciar RAM");
        return EXIT_FAILURE;
    }

    // 2. Iniciar Estructuras Administrativas
    if (memoria_core_init() != 0) {
        log_error(MEMORIA_CTX.logger, "Error al iniciar estructuras core");
        return EXIT_FAILURE;
    }

    // 3. Iniciar Frames (Bitmaps)
    if (frames_init() != 0) {
        log_error(MEMORIA_CTX.logger, "Error al iniciar frames");
        return EXIT_FAILURE;
    }

    // 4. Iniciar Swap (Files)
    if (swap_init() != 0) {
        log_error(MEMORIA_CTX.logger, "Error al iniciar swap");
        return EXIT_FAILURE;
    }

    // 5. Iniciar Server
    if (server_init(MEMORIA_CTX.config->puerto_escucha) != 0) {
        log_error(MEMORIA_CTX.logger, "Error al iniciar servidor");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void memoria_run(void) {
    log_info(MEMORIA_CTX.logger, "Memoria en ejecución (Listening)");
    server_listen_loop();
}

void memoria_shutdown(void) {
    log_info(MEMORIA_CTX.logger, "Apagando memoria...");
    server_shutdown();
    memoria_ram_destroy();
    frames_destroy();
    memoria_liberar_config(MEMORIA_CTX.config);
    
    if (MEMORIA_CTX.logger) log_destroy(MEMORIA_CTX.logger);
    if (MEMORIA_CTX.logger_error) log_destroy(MEMORIA_CTX.logger_error);
}

int get_tamanio_pagina(void) {
    return MEMORIA_CTX.config->tam_pagina;
}
