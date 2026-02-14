
#ifndef REGISTROS_H
#define REGISTROS_H

#include <model/model.h>

uint32_t registros_leer(registros_t* r, reg_id_t reg);
void registros_escribir(registros_t* r, reg_id_t reg, uint32_t valor);
void registros_reset(registros_t* r);
uint32_t registros_size(reg_id_t reg);

#endif