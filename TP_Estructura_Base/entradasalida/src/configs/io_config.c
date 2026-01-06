#include "io_config.h"
#include <string.h>

t_io_config* io_config_create(const char* path) {
    t_config* cfg = config_create(path);
    if (!cfg) return NULL;

    t_io_config* io_cfg = malloc(sizeof(t_io_config));

    char* type_str = config_get_string_value(cfg, "TIPO_INTERFAZ");
    io_cfg->tipo_interfaz = io_config_get_type(type_str);
    
    io_cfg->tiempo_unidad_trabajo = config_get_int_value(cfg, "TIEMPO_UNIDAD_TRABAJO");
    
    io_cfg->ip_kernel = strdup(config_get_string_value(cfg, "IP_KERNEL"));
    io_cfg->puerto_kernel = strdup(config_get_string_value(cfg, "PUERTO_KERNEL"));
    
    io_cfg->ip_memoria = strdup(config_get_string_value(cfg, "IP_MEMORIA"));
    io_cfg->puerto_memoria = strdup(config_get_string_value(cfg, "PUERTO_MEMORIA"));
    
    // Optional fields (DialFS)
    if (config_has_property(cfg, "PATH_BASE_DIALFS"))
        io_cfg->path_base_dialfs = strdup(config_get_string_value(cfg, "PATH_BASE_DIALFS"));
    else
        io_cfg->path_base_dialfs = NULL;
        
    if (config_has_property(cfg, "BLOCK_SIZE"))
        io_cfg->block_size = config_get_int_value(cfg, "BLOCK_SIZE");
    else
        io_cfg->block_size = 0;
        
    if (config_has_property(cfg, "BLOCK_COUNT"))
        io_cfg->block_count = config_get_int_value(cfg, "BLOCK_COUNT");
    else
        io_cfg->block_count = 0;

    config_destroy(cfg);
    return io_cfg;
}

void io_config_destroy(t_io_config* config) {
    if (!config) return;
    free(config->ip_kernel);
    free(config->puerto_kernel);
    free(config->ip_memoria);
    free(config->puerto_memoria);
    if (config->path_base_dialfs) free(config->path_base_dialfs);
    free(config);
}

t_io_type io_config_get_type(const char* type_str) {
    if (strcmp(type_str, "GENERICA") == 0) return IO_TYPE_GENERICA;
    if (strcmp(type_str, "STDIN") == 0) return IO_TYPE_STDIN;
    if (strcmp(type_str, "STDOUT") == 0) return IO_TYPE_STDOUT;
    if (strcmp(type_str, "DIALFS") == 0) return IO_TYPE_DIALFS;
    return IO_TYPE_GENERICA;
}
