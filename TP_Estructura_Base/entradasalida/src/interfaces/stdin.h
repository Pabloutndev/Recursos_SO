
#ifndef INTERFACES_STD_H_
#define INTERFACES_STD_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <readline/readline.h>
#include "../configs/io_config.h"
#include <utils/paquete.h>

void io_stdin_handler(t_list* packet, t_io_config* config, t_log* logger, int socket_memoria);
void io_stdout_handler(t_list* packet, t_io_config* config, t_log* logger, int socket_memoria);

#endif
