
#ifndef IO_MAIN_H_
#define IO_MAIN_H_

#include <config/io_config.h>
#include <commons/log.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>

typedef struct {
    t_log* logger;
    t_io_config* config;
    int socket_kernel;
    int socket_memoria;
    char* io_name;
} t_io_context;

extern t_io_context IO_CTX;

void io_receiver_loop();
void io_init(const char* config_path, char* name);

#endif
