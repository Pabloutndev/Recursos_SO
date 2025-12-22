#ifndef DECODER_H
#define DECODER_H

#include <instrucciones/instrucciones.h>

static reg_id_t parse_registro(char* r);
instruccion_t decoder_parsear(char* linea);

#endif