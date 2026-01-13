#ifndef COMMON_MEMORIA_RESPONSES_H
#define COMMON_MEMORIA_RESPONSES_H

#include <stdint.h>
#include <stdbool.h>

/* REQUEST */

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
} t_mem_read;

typedef struct {
    uint32_t pid;
    uint32_t direccion_fisica;
    uint32_t tamanio;
    void* buffer; // Pointer not serialized directly but holding data
} t_mem_write;

typedef struct {
    uint32_t pid;
    uint32_t pc;
} t_mem_fetch;

/* RESPONSE */

typedef struct {
    bool ok;
    uint32_t frame;
} t_mem_respuesta_traduccion;

typedef struct {
    bool ok;
    void* data; 
    uint32_t size;
} t_mem_respuesta_lectura;

#endif /* COMMON_MEMORIA_RESPONSES_H */