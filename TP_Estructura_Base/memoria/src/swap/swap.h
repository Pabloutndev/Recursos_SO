#ifndef SWAP_H_
#define SWAP_H_

#include <stdint.h>
#include <stdbool.h>

/* Init Swap (Crear directorio de ser necesario) */
int swap_init(void);

/* Operaciones de Swap */
bool swap_escribir_pagina(uint32_t pid, int nro_pagina, void* contenido);
bool swap_leer_pagina(uint32_t pid, int nro_pagina, void* buffer);
void swap_borrar_proceso(uint32_t pid);

#endif
