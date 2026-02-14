#include <mod_memoria.h>
#include <gestion/paginas.h>
#include <model/model.h>
#include <commons/collections/dictionary.h>
#include <string.h>
#include <pthread.h>

/* Reimplementacion de paginas.c usando la nueva estructura de frames */
t_dictionary* tablas_paginas = NULL;

static pthread_mutex_t mutex_paginas = PTHREAD_MUTEX_INITIALIZER;

static void check_init_diccionario() {
    if(!tablas_paginas) tablas_paginas = dictionary_create();
}

static char* pid_to_key(uint32_t pid) {
    char* key = malloc(20);
    snprintf(key, 20, "%u", pid);
    return key;
}

bool paginacion_crear_proceso(uint32_t pid, int tamanio_bytes) {
    pthread_mutex_lock(&mutex_paginas);
    check_init_diccionario();

    char* key = pid_to_key(pid);
    if (dictionary_has_key(tablas_paginas, key)) {
        free(key);
        pthread_mutex_unlock(&mutex_paginas);
        return false;
    }

    int tam_pag = MEMORIA_CTX.config->tam_pagina;
    int cant_paginas = tamanio_bytes / tam_pag;
    if (tamanio_bytes % tam_pag != 0) cant_paginas++;

    t_list* tabla = list_create();

    log_info(MEMORIA_CTX.logger, "PAGINACION: Recibido PID %u. Paginas requeridas: %d", pid, cant_paginas);

    for(int i=0; i<cant_paginas; i++) {
        t_pagina* pag = malloc(sizeof(t_pagina));
        pag->frame = -1;
        pag->presente = false;
        pag->modificado = false;
        pag->uso = false;
        list_add(tabla, pag);
    }

    dictionary_put(tablas_paginas, key, tabla);
    free(key);

    pthread_mutex_unlock(&mutex_paginas);
    return true;
}

void paginacion_destruir_proceso(uint32_t pid) {
    pthread_mutex_lock(&mutex_paginas);
    if (!tablas_paginas) {
        pthread_mutex_unlock(&mutex_paginas);
        return;
    }
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
    pthread_mutex_unlock(&mutex_paginas);
}

bool paginacion_resize(uint32_t pid, int nuevo_tamanio) {
    pthread_mutex_lock(&mutex_paginas);
    if (!tablas_paginas) {
        pthread_mutex_unlock(&mutex_paginas);
        return false;
    }

    char* key = pid_to_key(pid);
    t_list* tabla = dictionary_get(tablas_paginas, key);
    free(key);

    if (!tabla) {
        pthread_mutex_unlock(&mutex_paginas);
        return false;
    }

    int tam_pag = MEMORIA_CTX.config->tam_pagina;
    int paginas_actuales = list_size(tabla);
    int paginas_nuevas = nuevo_tamanio / tam_pag;
    if (nuevo_tamanio % tam_pag != 0) paginas_nuevas++;

    log_info(MEMORIA_CTX.logger, "RESIZE PID %u: %d paginas -> %d paginas", pid, paginas_actuales, paginas_nuevas);

    if (paginas_nuevas > paginas_actuales) {
        /* Agregar paginas nuevas */
        for (int i = paginas_actuales; i < paginas_nuevas; i++) {
            t_pagina* pag = malloc(sizeof(t_pagina));
            pag->frame = -1;
            pag->presente = false;
            pag->modificado = false;
            pag->uso = false;
            list_add(tabla, pag);
        }
    } else if (paginas_nuevas < paginas_actuales) {
        /* Eliminar paginas desde el final, liberando frames si estan presentes */
        for (int i = paginas_actuales - 1; i >= paginas_nuevas; i--) {
            t_pagina* pag = list_remove(tabla, i);
            if (pag->presente && pag->frame != -1) {
                liberar_frame(pag->frame);
            }
            free(pag);
        }
    }

    pthread_mutex_unlock(&mutex_paginas);
    return true;
}

t_pagina* paginacion_obtener_entrada(uint32_t pid, int nro_pagina)
{
    pthread_mutex_lock(&mutex_paginas);
    if (!tablas_paginas) {
        pthread_mutex_unlock(&mutex_paginas);
        return NULL;
    }

    char* key = pid_to_key(pid);
    t_list* tabla = dictionary_get(tablas_paginas, key);
    free(key);

    if (!tabla || nro_pagina >= list_size(tabla)) {
        pthread_mutex_unlock(&mutex_paginas);
        return NULL;
    }

    t_pagina* pagina = list_get(tabla, nro_pagina);

    /* ===== HIT ===== */
    if (pagina->presente) {
        pagina->uso = true;
        pthread_mutex_unlock(&mutex_paginas);
        return pagina;
    }

    /* ===== PAGE FAULT ===== */
    log_info(MEMORIA_CTX.logger, "PAGE FAULT PID %u PAG %d", pid, nro_pagina);

    int frame = obtener_frame_libre();

    if (frame == -1) {
        uint32_t pid_v;
        int pag_v;

        frame = elegir_victima_clock(&pid_v, &pag_v);
        if (frame == -1) {
            log_error(MEMORIA_CTX.logger, "MEMORIA Y SWAP LLENOS");
            pthread_mutex_unlock(&mutex_paginas);
            return NULL;
        }

        char* k = pid_to_key(pid_v);
        t_list* tabla_v = dictionary_get(tablas_paginas, k);
        free(k);

        if (!tabla_v) {
            log_error(MEMORIA_CTX.logger, "PAGINACION: Tabla victima PID %u no encontrada", pid_v);
            pthread_mutex_unlock(&mutex_paginas);
            return NULL;
        }

        t_pagina* victima = list_get(tabla_v, pag_v);
        int tam_pag = MEMORIA_CTX.config->tam_pagina;

        if (victima->modificado) {
            void* buffer = malloc(tam_pag);
            if (!buffer) {
                log_error(MEMORIA_CTX.logger, "PAGINACION: malloc failed para swap out");
                pthread_mutex_unlock(&mutex_paginas);
                return NULL;
            }
            leer_memoria_fisica(victima->frame * tam_pag, buffer, tam_pag);

            if (!swap_escribir_pagina(pid_v, pag_v, buffer)) {
                free(buffer);
                log_error(MEMORIA_CTX.logger, "SWAP LLENO - Abortando");
                pthread_mutex_unlock(&mutex_paginas);
                return NULL;
            }

            free(buffer);
        }

        victima->presente = false;
        victima->frame = -1;
        victima->modificado = false;
        victima->uso = false;
    }

    /* ===== SWAP IN ===== */
    int tam_pag = MEMORIA_CTX.config->tam_pagina;
    void* buffer = malloc(tam_pag);
    if (!buffer) {
        log_error(MEMORIA_CTX.logger, "PAGINACION: malloc failed para swap in");
        pthread_mutex_unlock(&mutex_paginas);
        return NULL;
    }

    if (!swap_leer_pagina(pid, nro_pagina, buffer))
        memset(buffer, 0, tam_pag);

    escribir_memoria_fisica(frame * tam_pag, buffer, tam_pag);
    free(buffer);

    pagina->frame = frame;
    pagina->presente = true;
    pagina->uso = true;
    pagina->modificado = false;

    log_info(MEMORIA_CTX.logger, "PAGINA CARGADA PID %u PAG %d -> FRAME %d", pid, nro_pagina, frame);

    pthread_mutex_unlock(&mutex_paginas);
    return pagina;
}

bool paginacion_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    uint32_t tam_pag = MEMORIA_CTX.config->tam_pagina;
    uint32_t bytes_escritos = 0;
    while (bytes_escritos < size) {
        uint32_t dir_actual = dir_logica + bytes_escritos;
        uint32_t num_pagina = dir_actual / tam_pag;
        uint32_t offset = dir_actual % tam_pag;
        uint32_t a_escribir = (tam_pag - offset < size - bytes_escritos) ? (tam_pag - offset) : (size - bytes_escritos);

        t_pagina* pag = paginacion_obtener_entrada(pid, num_pagina);
        if (!pag || !pag->presente) return false;

        pthread_mutex_lock(&mutex_paginas);
        escribir_memoria_fisica(pag->frame * tam_pag + offset, (char*)buffer + bytes_escritos, a_escribir);
        pag->uso = true;
        pag->modificado = true;
        pthread_mutex_unlock(&mutex_paginas);

        bytes_escritos += a_escribir;
    }
    return true;
}

bool paginacion_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    uint32_t tam_pag = MEMORIA_CTX.config->tam_pagina;
    uint32_t bytes_leidos = 0;
    while (bytes_leidos < size) {
        uint32_t dir_actual = dir_logica + bytes_leidos;
        uint32_t num_pagina = dir_actual / tam_pag;
        uint32_t offset = dir_actual % tam_pag;
        uint32_t a_leer = (tam_pag - offset < size - bytes_leidos) ? (tam_pag - offset) : (size - bytes_leidos);

        t_pagina* pag = paginacion_obtener_entrada(pid, num_pagina);
        if (!pag || !pag->presente) return false;

        pthread_mutex_lock(&mutex_paginas);
        leer_memoria_fisica(pag->frame * tam_pag + offset, (char*)buffer + bytes_leidos, a_leer);
        pag->uso = true;
        pthread_mutex_unlock(&mutex_paginas);

        bytes_leidos += a_leer;
    }
    return true;
}

char* paginacion_leer_instruccion(uint32_t pid, uint32_t pc)
{
    int tam_pag = MEMORIA_CTX.config->tam_pagina; 
    
    // ✅ UNIFICACIÓN: El PC es un índice, lo convertimos a dirección lógica (byte offset)
    uint32_t dir_logica = pc * 64;
    
    int pag_nro = dir_logica / tam_pag;
    int offset  = dir_logica % tam_pag;
    
    t_pagina* pag = paginacion_obtener_entrada(pid, pag_nro);
    
    if (pag && pag->presente && pag->frame != -1) {
        uint32_t dir_fisica = (pag->frame * tam_pag) + offset;
        
        char buffer[256]; 
        leer_memoria_fisica(dir_fisica, buffer, 255);
        buffer[255] = '\0'; 
        
        return strdup(buffer);
    } 
    
    return NULL; 
}
