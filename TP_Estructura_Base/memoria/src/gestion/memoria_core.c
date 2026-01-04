#include "memoria_core.h"
#include "../mod_memoria.h"
#include <string.h>

static void* espacio_memoria = NULL;
static int tam_memoria = 0;

int memoria_ram_init(void) {
    tam_memoria = memoria_config->tam_memoria;
    
    espacio_memoria = malloc(tam_memoria);
    if (!espacio_memoria) {
        log_error(logger, "Fallo malloc RAM");
        return -1;
    }
    memset(espacio_memoria, 0, tam_memoria);
    log_info(logger, "RAM reservada: %d bytes", tam_memoria);
    return 0;
}

void memoria_ram_destroy(void) {
    if (espacio_memoria) free(espacio_memoria);
}

bool leer_memoria_fisica(uint32_t dir_fisica, void* buffer, int tamanio) {
    if (dir_fisica + tamanio > tam_memoria) {
        log_error(logger, "Segmentation Fault (Read): %d", dir_fisica);
        return false;
    }
    memcpy(buffer, espacio_memoria + dir_fisica, tamanio);
    return true;
}

bool escribir_memoria_fisica(uint32_t dir_fisica, void* data, int tamanio) {
    if (dir_fisica + tamanio > tam_memoria) {
        log_error(logger, "Segmentation Fault (Write): %d", dir_fisica);
        return false;
    }
    memcpy(espacio_memoria + dir_fisica, data, tamanio);
    return true;
}

void* get_memoria_espacio(void) { return espacio_memoria; }
