#include <gestion/memoria_core.h>
#include <gestion/memoria_ram.h>
#include <frames/frames.h>
#include <swap/swap.h>
#include <server/server_mem.h>
#include <unistd.h>

t_log* logger;
t_log* loggerError;
t_memoria_config* memoria_config;

int memoria_init(const char* path_config) {
    logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
    if (!logger) return EXIT_FAILURE;

    log_info(logger, "Iniciando modulo Memoria...");

    memoria_config = memoria_cargar_config(path_config);
    if (!memoria_config) {
        log_error(logger, "Error cargar config");
        return EXIT_FAILURE;
    }

    // 1. Iniciar RAM (User Space)
    if (memoria_ram_init() != 0) {
        return EXIT_FAILURE;
    }

    // 2. Iniciar Estructuras Administrativas
    memoria_core_init();

    // 2. Iniciar Frames (Bitmaps)
    if (frames_init() != 0) {
        return EXIT_FAILURE;
    }

    // 3. Iniciar Swap (Files)
    if (swap_init() != 0) {
        return EXIT_FAILURE;
    }

    // 4. Server
    if (server_init(memoria_config->puerto_escucha) != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void memoria_run(void) {
    log_info(logger, "Memoria RUNNING");
    server_listen_loop();
}

void memoria_shutdown(void) {
    server_shutdown();
    memoria_ram_destroy();
    frames_destroy();
    memoria_liberar_config(memoria_config);
    log_destroy(logger);
}

int get_tamanio_pagina(void) {
    return memoria_config->tam_pagina;
}
