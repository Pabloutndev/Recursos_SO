#ifndef CONSOLA_CONFIG_H
#define CONSOLA_CONFIG_H

typedef struct {
    char* ip_kernel;
    char* puerto_kernel;
} t_consola_config;

t_consola_config consola_cargar_config(const char* path);

#endif
