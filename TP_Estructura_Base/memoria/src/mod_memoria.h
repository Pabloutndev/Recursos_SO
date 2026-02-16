#ifndef MOD_MEMORIA_H_
#define MOD_MEMORIA_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include "config/memoria_config.h"

/* Variables Globales */
typedef struct {
    t_log* logger;
    t_log* logger_error;
    t_memoria_config* config;
} t_memoria_context;

extern t_memoria_context MEMORIA_CTX;

/* Funciones de ciclo de vida */
int memoria_init(const char* path_config);
void memoria_run(void);
void memoria_shutdown(void);
int get_tamanio_pagina(void);

#endif
