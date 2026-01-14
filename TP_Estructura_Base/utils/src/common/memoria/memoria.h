#ifndef COMMON_MEMORIA_H
#define COMMON_MEMORIA_H

#include <stdint.h>
#include <stdbool.h>

/* REQUEST */

typedef struct {
    uint32_t pid;
    uint32_t tamanio;
    void* instrucciones;
    uint32_t size_instrucciones;
} t_mem_init_proceso;

typedef struct {
    uint32_t pid;
} t_mem_fin_proceso;

typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
} t_mem_traducir;

typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
    uint32_t size;
} t_mem_read;

typedef struct {
    uint32_t pid;
    uint32_t direccion_logica;
    uint32_t size;
    void* buffer;
} t_mem_write;

typedef struct {
    uint32_t pid;
    uint32_t pc;
} t_mem_fetch;

typedef struct {
    uint32_t pid;
    uint32_t nuevo_tamanio;
} t_mem_resize;


/* RESPONSE */

typedef struct {
    bool ok;
    uint32_t direccion_fisica;
} t_mem_respuesta_traduccion;

typedef struct {
    bool ok;
    void* data;
    uint32_t size;
} t_mem_respuesta_lectura;

#endif
