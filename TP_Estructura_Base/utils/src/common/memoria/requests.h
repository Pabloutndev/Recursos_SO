#ifndef COMMON_MEMORIA_REQUESTS_H
#define COMMON_MEMORIA_REQUESTS_H

#include <common/tipos_basicos.h>

typedef struct {
    pid_t pid;
    size_t_u tamanio;
} t_mem_init_proceso;

typedef struct {
    pid_t pid;
} t_mem_fin_proceso;

typedef struct {
    pid_t pid;
    uint32_t pagina;
} t_mem_traducir_pagina;

typedef struct {
    pid_t pid;
    addr_t direccion_fisica;
    size_t_u tamanio;
} t_mem_rw;

typedef struct {
    pid_t pid;
    addr_t pc;
} t_mem_fetch;

#endif
