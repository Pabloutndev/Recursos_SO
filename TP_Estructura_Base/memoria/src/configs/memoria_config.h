#ifndef MEMORIA_CONFIG_H_
#define MEMORIA_CONFIG_H_

#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <stdlib.h>

typedef struct {
    char* puerto_escucha;
    int tam_memoria;
    int tam_pagina;
    char* path_instrucciones;
    int retardo_respuesta;
    char* algoritmo_reemplazo;
} t_memoria_config;

t_memoria_config* memoria_cargar_config(const char* path);
void memoria_liberar_config(t_memoria_config* config);

#endif
