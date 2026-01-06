
#ifndef IO_MAIN_H_
#define IO_MAIN_H_

#include "../configs/io_config.h"
#include <commons/log.h>
#include <utils/conexion.h>
#include <utils/paquete.h>

void io_init(const char* config_path, char* name);

#endif
