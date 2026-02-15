#ifndef OPERACIONES_INTERNAL_H
#define OPERACIONES_INTERNAL_H

#include <instrucciones/instrucciones.h>
#include <model/model.h>

/* Funciones internas de operaciones con memoria.
 * Declaradas aqui para ser compartidas entre operaciones.c y operaciones_mem.c */

t_motivo_desalojo ejecutar_mov_in(instruccion_t* i, t_contexto_cpu* ctx);
t_motivo_desalojo ejecutar_mov_out(instruccion_t* i, t_contexto_cpu* ctx);
t_motivo_desalojo ejecutar_resize(instruccion_t* i, t_contexto_cpu* ctx);
t_motivo_desalojo ejecutar_copy_string(instruccion_t* i, t_contexto_cpu* ctx);

#endif
