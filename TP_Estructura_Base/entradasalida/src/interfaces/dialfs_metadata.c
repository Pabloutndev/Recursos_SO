
#include "dialfs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/string.h>

// ============================================================================
// Funciones de metadata / FCB
// ============================================================================

// Construye la ruta completa al archivo de metadata
char* metadata_path(const char* nombre) {
    return string_from_format("%s/metadata/%s.metadata", path_base, nombre);
}

// Carga un FCB (File Control Block) desde el archivo de metadata
t_dialfs_fcb load_fcb(const char* nombre) {
    t_dialfs_fcb fcb;
    memset(&fcb, 0, sizeof(t_dialfs_fcb));
    strncpy(fcb.nombre, nombre, sizeof(fcb.nombre) - 1);
    fcb.bloque_inicial = (uint32_t)-1;
    fcb.tamanio_archivo = 0;

    char* path = metadata_path(nombre);
    t_config* cfg = config_create(path);
    if (cfg) {
        if (config_has_property(cfg, "BLOQUE_INICIAL")) {
            int val = config_get_int_value(cfg, "BLOQUE_INICIAL");
            fcb.bloque_inicial = (uint32_t)val;
        }
        if (config_has_property(cfg, "TAMANIO_ARCHIVO")) {
            fcb.tamanio_archivo = (uint32_t)config_get_int_value(cfg, "TAMANIO_ARCHIVO");
        }
        config_destroy(cfg);
    }
    free(path);
    return fcb;
}

// Guarda un FCB en el archivo de metadata
void save_fcb(const t_dialfs_fcb* fcb) {
    char* path = metadata_path(fcb->nombre);
    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "BLOQUE_INICIAL=%d\n", (int)fcb->bloque_inicial);
        fprintf(f, "TAMANIO_ARCHIVO=%u\n", fcb->tamanio_archivo);
        fclose(f);
    } else {
        log_error(fs_logger, "FS: No se pudo guardar metadata en %s", path);
    }
    free(path);
}

// Verifica si existe el archivo de metadata para un nombre dado
bool metadata_exists(const char* nombre) {
    char* path = metadata_path(nombre);
    FILE* f = fopen(path, "r");
    free(path);
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}
