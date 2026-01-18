#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <instrucciones/instrucciones.h>
#include <model/model.h>

static void ejecutar_set(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sum(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sub(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_jnz(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_exit(t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_mov_out(instruccion_t* i, t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_mov_in(instruccion_t* i, t_contexto_cpu* ctx);

t_motivo_desalojo execute_instruccion(instruccion_t* inst, t_contexto_cpu* ctx);

#endif
