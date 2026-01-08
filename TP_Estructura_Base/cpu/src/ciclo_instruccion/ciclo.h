#ifndef CICLO_H
#define CICLO_H

#include <contexto/contexto.h>
#include <instrucciones/instrucciones.h>

/**
 * @brief Se encarga de ejecutar el ciclo de cpu (FETCH, DECODE, EXECUTE) 
 * @param t_contexto_cpu* ctx
 * @return void
*/
void ciclo_instruccion_ejecutar(t_contexto_cpu* ctx);
char* fetch_instruccion(t_contexto_cpu* ctx);
instruccion_t decode_instruccion(const char* linea);

#endif