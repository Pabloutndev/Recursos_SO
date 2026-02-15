
#ifndef PAGINAS_INTERNAL_H_
#define PAGINAS_INTERNAL_H_

#include <gestion/paginas.h>
#include <pthread.h>

// ============================================================================
// Estado compartido (definido en paginas.c)
// ============================================================================

extern pthread_mutex_t mutex_paginas;

// ============================================================================
// Funciones de I/O de paginas (paginas_io.c)
// ============================================================================

bool paginacion_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);
bool paginacion_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);
char* paginacion_leer_instruccion(uint32_t pid, uint32_t pc);

#endif
