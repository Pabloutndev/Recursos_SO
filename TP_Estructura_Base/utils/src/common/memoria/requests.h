#ifndef COMMON_MEMORIA_REQUESTS_H
#define COMMON_MEMORIA_REQUESTS_H

#include <common/tipos_basicos.h>
#include <stdint.h>

typedef struct {
    uint32_t pid;
    uint32_t tamanio;
} t_mem_init_proceso;

typedef struct {
    uint32_t pid;
} t_mem_fin_proceso;

typedef struct {
    uint32_t pid;
    uint32_t pagina;
} t_mem_traducir_pagina;

typedef struct {
    uint32_t pid;
    uint32_t direccion_fisica;
    uint32_t tamanio;
} t_mem_rw;

typedef struct {
    uint32_t pid;
    uint32_t pc;
} t_mem_fetch;

#endif
