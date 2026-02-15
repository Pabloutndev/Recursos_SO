#ifndef ESQUEMA_MEMORIA_H_
#define ESQUEMA_MEMORIA_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum { ESQUEMA_PAGINACION, ESQUEMA_SEGMENTACION } t_esquema;

/* Inicializacion - se llama una vez al arrancar con el esquema elegido */
void esquema_memoria_init(t_esquema esquema);
t_esquema esquema_memoria_actual(void);

/* Gestion de procesos */
bool esquema_crear_proceso(uint32_t pid, uint32_t tamanio);
void esquema_destruir_proceso(uint32_t pid);
bool esquema_resize(uint32_t pid, uint32_t nuevo_tamanio);

/* Acceso logico a memoria */
bool esquema_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);
bool esquema_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size);

/* Fetch de instrucciones */
char* esquema_leer_instruccion(uint32_t pid, uint32_t pc);

/* Traduccion de direccion logica a fisica. Retorna la dir fisica o -1 si error */
int64_t esquema_traducir(uint32_t pid, uint32_t dir_logica);

#endif
