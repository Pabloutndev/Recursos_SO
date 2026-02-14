#ifndef PAGINAS_H_
#define PAGINAS_H_

#include <stdint.h>
#include <commons/collections/list.h>

typedef struct {
    int frame;
    bool presente;
    bool modificado;
    bool uso;
    // swap info si hiciera falta
} t_pagina;

/* Gestión de Creación/Destrucción de Procesos (Paginación) */
bool paginacion_crear_proceso(uint32_t pid, int tamanio_bytes);
void paginacion_destruir_proceso(uint32_t pid);
bool paginacion_resize(uint32_t pid, int nuevo_tamanio);

/* Acceso a Tablas */
t_pagina* paginacion_obtener_entrada(uint32_t pid, int nro_pagina);

/* Acceso Lógico a Memoria (vía paginación) */
bool paginacion_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);
bool paginacion_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);

/* Lectura de Instrucciones (Fetch) */
char* paginacion_leer_instruccion(uint32_t pid, uint32_t pc);

/* Debug */
void dump_paginas(uint32_t pid);

#endif
