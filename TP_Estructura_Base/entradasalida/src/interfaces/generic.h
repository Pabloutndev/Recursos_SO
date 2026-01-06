
#ifndef INTERFACES_GENERIC_H_
#define INTERFACES_GENERIC_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <unistd.h>
#include "../configs/io_config.h"

void io_generic_handler(t_list* packet, t_io_config* config, t_log* logger);

#endif
