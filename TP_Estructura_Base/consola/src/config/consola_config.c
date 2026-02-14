#include <config/consola_config.h>
#include <commons/config.h>
#include <stdlib.h>
#include <string.h>

t_consola_config consola_cargar_config(const char* path) {
    t_consola_config cfg;
    t_config* config = config_create((char*)path);

    if (config == NULL) {
        perror("Archivo de configuracion de consola no encontrado");
        exit(EXIT_FAILURE);
    }

    cfg.ip_kernel = strdup(config_get_string_value(config, "IP_KERNEL"));
    cfg.puerto_kernel = strdup(config_get_string_value(config, "PUERTO_KERNEL"));

    config_destroy(config);
    return cfg;
}
