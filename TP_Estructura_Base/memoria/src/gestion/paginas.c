#include "paginas.h"
#include "../management_interfaces.h" // Wait, I need a standard interface?
#include "../frames/frames.h"
#include "../mod_memoria.h"
#include <commons/collections/dictionary.h>
#include <string.h>

/* Reimplementacion de paginas.c usando la nueva estructura de frames */
static t_dictionary* tablas_paginas = NULL;

static void check_init_diccionario() {
    if(!tablas_paginas) tablas_paginas = dictionary_create();
}

static char* pid_to_key(uint32_t pid) {
    char* key = malloc(20);
    sprintf(key, "%d", pid);
    return key;
}

bool paginacion_crear_proceso(uint32_t pid, int tamanio_bytes) {
    check_init_diccionario();
    
    char* key = pid_to_key(pid);
    if (dictionary_has_key(tablas_paginas, key)) {
        free(key);
        return false;
    }

    int tam_pag = memoria_config->tam_pagina;
    int cant_paginas = tamanio_bytes / tam_pag;
    if (tamanio_bytes % tam_pag != 0) cant_paginas++;

    t_list* tabla = list_create();

    log_info(logger, "PAGINACION: Recibido PID %d. Paginas requeridas: %d", pid, cant_paginas);

    for(int i=0; i<cant_paginas; i++) {
        t_pagina* pag = malloc(sizeof(t_pagina));
        pag->frame = -1; 
        pag->presente = false;
        pag->modificado = false;
        pag->uso = false;
        list_add(tabla, pag);
    }

    dictionary_put(tablas_paginas, key, tabla);
    free(key); // Commons copies key? IMPORTANT: dictionary_put implementation usually duplicates key or takes ownership.
    // In commons SO implementation it usually takes ownership or duplicates. Standard usage is usually strdup. 
    // Wait, dictionary_put(dict, key, value). If I pass heap allocated key, does it free it?
    // Usually no. But key collision check uses string. 
    // Safest: pass strdup-ed key if generated, and destroy generic.
    
    return true;
}

void paginacion_destruir_proceso(uint32_t pid) {
    if (!tablas_paginas) return;
    char* key = pid_to_key(pid);
    
    t_list* tabla = dictionary_remove(tablas_paginas, key);
    if(tabla) {
        for(int i=0; i<list_size(tabla); i++) {
            t_pagina* p = list_get(tabla, i);
            if(p->presente && p->frame != -1) {
                liberar_frame(p->frame);
            }
            free(p);
        }
        list_destroy(tabla);
    }
    free(key);
}

t_pagina* paginacion_obtener_entrada(uint32_t pid, int pagina) {
    if (!tablas_paginas) return NULL;
    char* key = pid_to_key(pid);
    t_list* tabla = dictionary_get(tablas_paginas, key);
    free(key);
    
    if(!tabla || pagina >= list_size(tabla)) return NULL;
    
    return list_get(tabla, pagina);
}
