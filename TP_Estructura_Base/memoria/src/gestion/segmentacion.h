#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include <stdint.h>
#include <stdbool.h>

/* Estructura de segmento */
typedef struct {
    uint32_t base;
    uint32_t limite;
} t_segmento;

/* Gestion de procesos */
bool segmentacion_crear_proceso(uint32_t pid, int tamanio_bytes);
void segmentacion_destruir_proceso(uint32_t pid);
bool segmentacion_resize(uint32_t pid, int nuevo_tamanio);

/* Acceso logico a memoria */
bool segmentacion_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);
bool segmentacion_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);

/* Fetch de instrucciones */
char* segmentacion_leer_instruccion(uint32_t pid, uint32_t pc);

/* Traduccion: retorna dir_fisica, 0 si error (segmentation fault) */
uint32_t segmentacion_traducir(uint32_t pid, uint32_t dir_logica);

#endif
