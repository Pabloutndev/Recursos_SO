#ifndef MEMORIA_CORE_H_
#define MEMORIA_CORE_H_

#include <stdint.h>
#include <stdbool.h>

/* Init/Destroy del USER SPACE (RAM) */
int memoria_ram_init(void); // Renamed from core_init for clarity? let's keep it simple
void memoria_ram_destroy(void);

/* Acceso TIPO RAM */
bool leer_memoria_fisica(uint32_t dir_fisica, void* buffer, int tamanio);
bool escribir_memoria_fisica(uint32_t dir_fisica, void* data, int tamanio);

/// NOTE: SOLO PARA DEBUG - Funcion Auxiliar
/// Dangerous but sometimes needed
void* get_memoria_espacio(void);

#endif
