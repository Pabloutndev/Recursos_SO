#ifndef MOD_MEMORIA_H_
#define MOD_MEMORIA_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include "configs/memoria_config.h"

/* Variables Globales */
extern t_log* logger;
extern t_memoria_config* memoria_config;

/* Funciones de ciclo de vida */

int memoria_init(const char* path_config);
void memoria_run(void);
void memoria_shutdown(void);
int get_tamanio_pagina(void);

#endif
