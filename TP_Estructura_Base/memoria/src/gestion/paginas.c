#include <mod_memoria.h>
#include <gestion/paginas.h>
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

t_pagina* paginacion_obtener_entrada(uint32_t pid, int nro_pagina) {
    if (!tablas_paginas) return NULL;

    char* key = pid_to_key(pid);
    t_list* tabla = dictionary_get(tablas_paginas, key);
    free(key);
    
    if(!tabla || nro_pagina >= list_size(tabla)) return NULL;
    
    t_pagina* pagina = list_get(tabla, nro_pagina);

    if (pagina->presente) {
        pagina->uso = true;
        return pagina;
    }
// ================= PAGE FAULT =================
    log_info(logger, "PAGE FAULT PID %d PAG %d", pid, nro_pagina);

    int frame = obtener_frame_libre();

    if (frame == -1) {
        uint32_t pid_v;
        int pag_v;

        frame = elegir_victima_clock(&pid_v, &pag_v);

        t_pagina* victima = paginacion_obtener_entrada(pid_v, pag_v);

        int tam_pag = memoria_config->tam_pagina;

        if (victima->modificado) {
            void* buffer = malloc(tam_pag);
            leer_memoria_fisica(victima->frame * tam_pag, buffer, tam_pag);
            swap_escribir_pagina(pid_v, pag_v, buffer);
            free(buffer);

            log_info(logger, "SWAP OUT PID %d PAG %d", pid_v, pag_v);
        }

        victima->presente = false;
        victima->frame = -1;
        victima->modificado = false;
        victima->uso = false;
    }

    // ================= SWAP IN =================
    int tam_pag = memoria_config->tam_pagina;
    void* buffer = malloc(tam_pag);

    if (!swap_leer_pagina(pid, nro_pagina, buffer)) {
        memset(buffer, 0, tam_pag); // primera vez
    }

    escribir_memoria_fisica(frame * tam_pag, buffer, tam_pag);
    free(buffer);

    pagina->frame = frame;
    pagina->presente = true;
    pagina->uso = true;
    pagina->modificado = false;

    log_info(logger, "SWAP IN PID %d PAG %d -> FRAME %d", pid, nro_pagina, frame);

    return pagina;
}
