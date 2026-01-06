
#ifndef INTERFACES_DIALFS_H_
#define INTERFACES_DIALFS_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "../configs/io_config.h"
#include <utils/paquete.h>

void io_dialfs_init(t_io_config* config, t_log* logger);
void io_dialfs_handler(int op_code, t_list* packet, t_io_config* config, t_log* logger, int socket_memoria);

#endif
