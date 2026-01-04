#include "memoria_config.h"

t_memoria_config* memoria_cargar_config(const char* path) {
    t_config* cfg = config_create(path);
    if(cfg == NULL) {
        return NULL;
    }

    t_memoria_config* config = malloc(sizeof(t_memoria_config));

    config->puerto_escucha = strdup(config_get_string_value(cfg, "PUERTO_ESCUCHA"));
    config->tam_memoria = config_get_int_value(cfg, "TAM_MEMORIA");
    config->tam_pagina = config_get_int_value(cfg, "TAM_PAGINA");
    config->path_instrucciones = strdup(config_get_string_value(cfg, "PATH_INSTRUCCIONES"));
    config->retardo_respuesta = config_get_int_value(cfg, "RETARDO_RESPUESTA");
    config->algoritmo_reemplazo = strdup(config_get_string_value(cfg, "ALGORITMO_REEMPLAZO"));

    config_destroy(cfg);
    return config;
}

void memoria_liberar_config(t_memoria_config* config) {
    if(config) {
        free(config->puerto_escucha);
        free(config->path_instrucciones);
        free(config->algoritmo_reemplazo);
        free(config);
    }
}
