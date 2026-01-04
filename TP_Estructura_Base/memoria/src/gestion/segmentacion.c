#include "segmentacion.h"
#include "../mod_memoria.h"
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>

/* Estructura de Segmento (Base / Limite) */
typedef struct {
    uint32_t base;
    uint32_t limite;
} t_segmento;

static t_dictionary* tablas_segmentos = NULL; // PID -> List<t_segmento*>

static void check_init_seg() {
    if (!tablas_segmentos) tablas_segmentos = dictionary_create();
}

bool segmentacion_crear_proceso(uint32_t pid, int size_bytes) {
    check_init_seg();
    
    // Logica MOCK: Asignar un segmento contiguo para todo el proceso (Base: 0 + offset pid*size ??? No real)
    // En segmentacion pura buscariamos huecos libres en memoria_ram.
    // Como esto es un Stub "Universal" para cuando se pida, dejaremos el esqueleto.
    
    t_list* segmentos = list_create();
    t_segmento* seg_code = malloc(sizeof(t_segmento));
    
    // Hardcoded logic: cada proceso tiene base = pid * 1000 (Solo demo)
    seg_code->base = pid * 1000; 
    seg_code->limite = size_bytes;
    
    list_add(segmentos, seg_code);
    
    char key[20]; sprintf(key, "%d", pid);
    dictionary_put(tablas_segmentos, strdup(key), segmentos);
    
    log_info(logger, "SEGMENTACION: Proceso %d creado (Base MOCK: %d)", pid, seg_code->base);
    return true;
}

void segmentacion_destruir_proceso(uint32_t pid) {
    if (!tablas_segmentos) return;
    char key[20]; sprintf(key, "%d", pid);
    t_list* segs = dictionary_remove(tablas_segmentos, key);
    if (segs) {
        list_destroy_and_destroy_elements(segs, free);
    }
}

uint32_t segmentacion_traducir(uint32_t pid, uint32_t dir_logica) {
    // Asumimos dir_logica = offset dentro del unico segmento code (Simple Segmentation)
    // O si hubiera ID de segmento en bits altos...
    
    // Implementacion Base: Base + Offset
    if (!tablas_segmentos) return 0;
    
    char key[20]; sprintf(key, "%d", pid);
    t_list* segs = dictionary_get(tablas_segmentos, key);
    
    if (!segs || list_is_empty(segs)) return 0;
    
    // Tomar segmento 0
    t_segmento* seg = list_get(segs, 0);
    
    if (dir_logica > seg->limite) {
        log_error(logger, "SEGMENTATION FAULT PID %d", pid);
        return 0; // Error
    }
    
    return seg->base + dir_logica;
}
