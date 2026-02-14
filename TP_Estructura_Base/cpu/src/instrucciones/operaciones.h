#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <instrucciones/instrucciones.h>
#include <model/model.h>

t_motivo_desalojo execute_instruccion(instruccion_t* inst, t_contexto_cpu* ctx);

#endif
