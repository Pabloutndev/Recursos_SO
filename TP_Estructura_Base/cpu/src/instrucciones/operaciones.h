#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <instrucciones/instrucciones.h>
#include <contexto/contexto.h>

static void ejecutar_set(instruccion_t* i, contexto_t* ctx);
static void ejecutar_sum(instruccion_t* i, contexto_t* ctx);
static void ejecutar_sub(instruccion_t* i, contexto_t* ctx);
static void ejecutar_jnz(instruccion_t* i, contexto_t* ctx);
static void ejecutar_io(instruccion_t* i, contexto_t* ctx);
static void ejecutar_exit(contexto_t* ctx);

void execute_instruccion(instruccion_t* inst, void* contexto);

#endif
