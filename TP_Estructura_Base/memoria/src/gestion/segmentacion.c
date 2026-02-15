#include <gestion/segmentacion.h>
#include <gestion/memoria_ram.h>
#include <mod_memoria.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

extern t_log* logger;
extern t_log* loggerError;

typedef struct {
    uint32_t base;
    uint32_t tamanio;
} t_bloque_libre;

static t_dictionary* tablas_segmentos = NULL;
static t_list* lista_libres = NULL;
static pthread_mutex_t mutex_seg = PTHREAD_MUTEX_INITIALIZER;

/* Forward declarations for internal helpers */
static t_bloque_libre* buscar_bloque_first_fit(uint32_t tamanio);
static void devolver_bloque(uint32_t base, uint32_t tamanio);
static void merge_bloques_adyacentes(void);
static char* seg_pid_key(uint32_t pid);

/* Initialize free list - called lazily on first use */
static void seg_init_if_needed(void) {
    if (tablas_segmentos != NULL) return;
    tablas_segmentos = dictionary_create();
    lista_libres = list_create();
    /* One big free block covering all RAM */
    t_bloque_libre* bloque = malloc(sizeof(t_bloque_libre));
    bloque->base = 0;
    bloque->tamanio = MEMORIA_CTX.config->tam_memoria;
    list_add(lista_libres, bloque);
    log_info(logger, "Memoria: segmentacion init, RAM libre: %d bytes", bloque->tamanio);
}

bool segmentacion_crear_proceso(uint32_t pid, int tamanio_bytes) {
    pthread_mutex_lock(&mutex_seg);
    seg_init_if_needed();

    char* key = seg_pid_key(pid);
    if (dictionary_has_key(tablas_segmentos, key)) {
        log_warning(logger, "PID: %u - Proceso ya existe", pid);
        free(key);
        pthread_mutex_unlock(&mutex_seg);
        return false;
    }

    t_bloque_libre* bloque = buscar_bloque_first_fit(tamanio_bytes);
    if (!bloque) {
        log_error(loggerError, "PID: %u - Sin espacio contiguo (%d bytes)", pid, tamanio_bytes);
        free(key);
        pthread_mutex_unlock(&mutex_seg);
        return false;
    }

    t_segmento* seg = malloc(sizeof(t_segmento));
    seg->base = bloque->base;
    seg->limite = tamanio_bytes;

    /* Split the free block: remove allocated portion from front */
    if ((uint32_t)tamanio_bytes == bloque->tamanio) {
        /* Exact fit - remove block from free list */
        for (int i = 0; i < list_size(lista_libres); i++) {
            if (list_get(lista_libres, i) == bloque) {
                list_remove(lista_libres, i);
                free(bloque);
                break;
            }
        }
    } else {
        /* Partial fit - shrink the free block */
        bloque->base += tamanio_bytes;
        bloque->tamanio -= tamanio_bytes;
    }

    dictionary_put(tablas_segmentos, key, seg);
    log_info(logger, "PID: %u - Creado base=%u limite=%u", pid, seg->base, seg->limite);

    pthread_mutex_unlock(&mutex_seg);
    return true;
}

void segmentacion_destruir_proceso(uint32_t pid) {
    pthread_mutex_lock(&mutex_seg);
    if (!tablas_segmentos) { pthread_mutex_unlock(&mutex_seg); return; }

    char* key = seg_pid_key(pid);
    t_segmento* seg = dictionary_remove(tablas_segmentos, key);
    free(key);

    if (seg) {
        devolver_bloque(seg->base, seg->limite);
        merge_bloques_adyacentes();
        log_info(logger, "PID: %u - Destruido, liberados %u bytes base=%u", pid, seg->limite, seg->base);
        free(seg);
    }

    pthread_mutex_unlock(&mutex_seg);
}

bool segmentacion_resize(uint32_t pid, int nuevo_tamanio) {
    pthread_mutex_lock(&mutex_seg);
    if (!tablas_segmentos) { pthread_mutex_unlock(&mutex_seg); return false; }

    char* key = seg_pid_key(pid);
    t_segmento* seg = dictionary_get(tablas_segmentos, key);
    free(key);

    if (!seg) { pthread_mutex_unlock(&mutex_seg); return false; }

    if ((uint32_t)nuevo_tamanio == seg->limite) {
        pthread_mutex_unlock(&mutex_seg);
        return true;
    }

    if (nuevo_tamanio < (int)seg->limite) {
        /* Shrink: free the tail */
        uint32_t diff = seg->limite - nuevo_tamanio;
        devolver_bloque(seg->base + nuevo_tamanio, diff);
        merge_bloques_adyacentes();
        seg->limite = nuevo_tamanio;
        log_info(logger, "PID: %u - Reducido a %d bytes", pid, nuevo_tamanio);
        pthread_mutex_unlock(&mutex_seg);
        return true;
    }

    /* Grow: try to extend in place first */
    uint32_t extra = nuevo_tamanio - seg->limite;
    uint32_t end = seg->base + seg->limite;

    /* Check if there's an adjacent free block right after this segment */
    bool extended = false;
    for (int i = 0; i < list_size(lista_libres); i++) {
        t_bloque_libre* bl = list_get(lista_libres, i);
        if (bl->base == end && bl->tamanio >= extra) {
            /* Can extend in place */
            if (bl->tamanio == extra) {
                list_remove(lista_libres, i);
                free(bl);
            } else {
                bl->base += extra;
                bl->tamanio -= extra;
            }
            seg->limite = nuevo_tamanio;
            extended = true;
            break;
        }
    }

    if (!extended) {
        /* Need to relocate: allocate new block, copy data, free old */
        t_bloque_libre* nuevo = buscar_bloque_first_fit(nuevo_tamanio);
        if (!nuevo) {
            log_error(loggerError, "PID: %u - Sin espacio para resize a %d bytes", pid, nuevo_tamanio);
            pthread_mutex_unlock(&mutex_seg);
            return false;
        }

        uint32_t nueva_base = nuevo->base;
        if ((uint32_t)nuevo_tamanio == nuevo->tamanio) {
            for (int i = 0; i < list_size(lista_libres); i++) {
                if (list_get(lista_libres, i) == nuevo) {
                    list_remove(lista_libres, i);
                    free(nuevo);
                    break;
                }
            }
        } else {
            nuevo->base += nuevo_tamanio;
            nuevo->tamanio -= nuevo_tamanio;
        }

        /* Copy old data to new location using raw RAM */
        void* temp = malloc(seg->limite);
        leer_memoria_fisica(seg->base, temp, seg->limite);
        escribir_memoria_fisica(nueva_base, temp, seg->limite);
        free(temp);

        /* Free old block */
        devolver_bloque(seg->base, seg->limite);
        merge_bloques_adyacentes();

        seg->base = nueva_base;
        seg->limite = nuevo_tamanio;
    }

    log_info(logger, "PID: %u - Resize a %d bytes base=%u", pid, nuevo_tamanio, seg->base);
    pthread_mutex_unlock(&mutex_seg);
    return true;
}

uint32_t segmentacion_traducir(uint32_t pid, uint32_t dir_logica) {
    pthread_mutex_lock(&mutex_seg);
    if (!tablas_segmentos) { pthread_mutex_unlock(&mutex_seg); return 0; }

    char* key = seg_pid_key(pid);
    t_segmento* seg = dictionary_get(tablas_segmentos, key);
    free(key);

    if (!seg) { pthread_mutex_unlock(&mutex_seg); return 0; }

    if (dir_logica >= seg->limite) {
        log_error(loggerError, "PID: %u - Segfault dir=%u >= limite=%u", pid, dir_logica, seg->limite);
        pthread_mutex_unlock(&mutex_seg);
        return 0;
    }

    uint32_t resultado = seg->base + dir_logica;
    pthread_mutex_unlock(&mutex_seg);
    return resultado;
}

bool segmentacion_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    pthread_mutex_lock(&mutex_seg);
    if (!tablas_segmentos) { pthread_mutex_unlock(&mutex_seg); return false; }

    char* key = seg_pid_key(pid);
    t_segmento* seg = dictionary_get(tablas_segmentos, key);
    free(key);

    if (!seg) { pthread_mutex_unlock(&mutex_seg); return false; }
    if (dir_logica + size > seg->limite) {
        log_error(loggerError, "PID: %u - Segfault write dir=%u size=%u limite=%u", pid, dir_logica, size, seg->limite);
        pthread_mutex_unlock(&mutex_seg);
        return false;
    }

    uint32_t dir_fisica = seg->base + dir_logica;
    pthread_mutex_unlock(&mutex_seg);
    return escribir_memoria_fisica(dir_fisica, buffer, size);
}

bool segmentacion_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    pthread_mutex_lock(&mutex_seg);
    if (!tablas_segmentos) { pthread_mutex_unlock(&mutex_seg); return false; }

    char* key = seg_pid_key(pid);
    t_segmento* seg = dictionary_get(tablas_segmentos, key);
    free(key);

    if (!seg) { pthread_mutex_unlock(&mutex_seg); return false; }
    if (dir_logica + size > seg->limite) {
        log_error(loggerError, "PID: %u - Segfault read dir=%u size=%u limite=%u", pid, dir_logica, size, seg->limite);
        pthread_mutex_unlock(&mutex_seg);
        return false;
    }

    uint32_t dir_fisica = seg->base + dir_logica;
    pthread_mutex_unlock(&mutex_seg);
    return leer_memoria_fisica(dir_fisica, buffer, size);
}

char* segmentacion_leer_instruccion(uint32_t pid, uint32_t pc) {
    /* Each instruction occupies 64 bytes (same convention as paginacion) */
    uint32_t dir_logica = pc * 64;
    char buffer[64];
    memset(buffer, 0, 64);

    if (!segmentacion_leer(pid, dir_logica, buffer, 64)) {
        return NULL;
    }

    return strdup(buffer);
}

/* ============ HELPERS INTERNOS ============ */

static t_bloque_libre* buscar_bloque_first_fit(uint32_t tamanio) {
    for (int i = 0; i < list_size(lista_libres); i++) {
        t_bloque_libre* bl = list_get(lista_libres, i);
        if (bl->tamanio >= tamanio) return bl;
    }
    return NULL;
}

static void devolver_bloque(uint32_t base, uint32_t tamanio) {
    t_bloque_libre* bl = malloc(sizeof(t_bloque_libre));
    bl->base = base;
    bl->tamanio = tamanio;

    /* Insert sorted by base address */
    bool inserted = false;
    for (int i = 0; i < list_size(lista_libres); i++) {
        t_bloque_libre* existente = list_get(lista_libres, i);
        if (base < existente->base) {
            list_add_in_index(lista_libres, i, bl);
            inserted = true;
            break;
        }
    }
    if (!inserted) list_add(lista_libres, bl);
}

static void merge_bloques_adyacentes(void) {
    int i = 0;
    while (i < list_size(lista_libres) - 1) {
        t_bloque_libre* actual = list_get(lista_libres, i);
        t_bloque_libre* siguiente = list_get(lista_libres, i + 1);
        if (actual->base + actual->tamanio == siguiente->base) {
            actual->tamanio += siguiente->tamanio;
            list_remove(lista_libres, i + 1);
            free(siguiente);
            /* Don't increment i, check again with the next block */
        } else {
            i++;
        }
    }
}

static char* seg_pid_key(uint32_t pid) {
    return string_itoa(pid);
}
