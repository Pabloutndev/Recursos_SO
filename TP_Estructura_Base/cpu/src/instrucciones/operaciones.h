#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <instrucciones/instrucciones.h>
#include <common/cpu/contexto.h>

static void ejecutar_set(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sum(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sub(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_jnz(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_exit(t_contexto_cpu* ctx);

t_cpu_motivo execute_instruccion(instruccion_t* inst, t_contexto_cpu* ctx);

#endif
