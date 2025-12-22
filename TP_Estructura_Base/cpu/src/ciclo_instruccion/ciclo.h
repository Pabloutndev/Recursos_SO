#ifndef CICLO_H
#define CICLO_H

#include <contexto/contexto.h>
#include <instrucciones/instrucciones.h>

/**
 * @brief Se encarga de ejecutar el ciclo de cpu (FETCH, DECODE, EXECUTE) 
 * @param contexto_t* ctx
 * @return void
*/
void ciclo_instruccion_ejecutar(contexto_t* ctx);
char* fetch_instruccion(contexto_t* ctx);
instruccion_t decode_instruccion(const char* linea);

#endif