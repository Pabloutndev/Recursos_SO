
#ifndef IO_CONFIG_H_
#define IO_CONFIG_H_

#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <stdlib.h>

typedef enum {
    IO_TYPE_GENERICA,
    IO_TYPE_STDIN,
    IO_TYPE_STDOUT,
    IO_TYPE_DIALFS
} t_io_type;

typedef struct {
    t_io_type tipo_interfaz;
    int tiempo_unidad_trabajo;
    char* ip_kernel;
    char* puerto_kernel; // String for getaddrinfo
    char* ip_memoria;
    char* puerto_memoria;
    char* path_base_dialfs;
    int block_size;
    int block_count;
} t_io_config;

t_io_config* io_config_create(const char* path);
void io_config_destroy(t_io_config* config);

// Helper to convert string to enum
t_io_type io_config_get_type(const char* type_str);

#endif
