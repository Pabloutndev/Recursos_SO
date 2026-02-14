#include <frames/frames.h>
#include <mod_memoria.h>
#include <commons/bitarray.h>
#include <stdlib.h>
#include <pthread.h>

static t_bitarray* frames_bitmap = NULL;
static void* frames_bits = NULL;
static int cantidad_marcos = 0;

static pthread_mutex_t mutex_frames = PTHREAD_MUTEX_INITIALIZER;

int frames_init(void) {
    int tam_memoria = MEMORIA_CTX.config->tam_memoria;
    int tam_pagina = MEMORIA_CTX.config->tam_pagina;

    if (tam_pagina <= 0) return -1;
    cantidad_marcos = tam_memoria / tam_pagina;

    // Bitmap
    size_t bytes_bitmap = (cantidad_marcos / 8); 
    if (cantidad_marcos % 8 != 0) bytes_bitmap++;

    frames_bits = malloc(bytes_bitmap);
    if (!frames_bits) return -1;

    frames_bitmap = bitarray_create_with_mode(frames_bits, bytes_bitmap, LSB_FIRST);

    // Limpiar (0 = Libre)
    for(int i=0; i<cantidad_marcos; i++) {
        bitarray_clean_bit(frames_bitmap, i);
    }
    
    log_info(MEMORIA_CTX.logger, "Frames Manager: %d Frames gestionados.", cantidad_marcos);
    return 0;
}

void frames_destroy(void) {
    if (frames_bitmap) bitarray_destroy(frames_bitmap);
    if (frames_bits) free(frames_bits);
}

int obtener_frame_libre(void) {
    pthread_mutex_lock(&mutex_frames);
    for(int i=0; i<cantidad_marcos; i++) {
        if (!bitarray_test_bit(frames_bitmap, i)) {
            bitarray_set_bit(frames_bitmap, i);
            pthread_mutex_unlock(&mutex_frames);
            return i;
        }
    }
    pthread_mutex_unlock(&mutex_frames);
    return -1; // -1 significa memoria llena (necesita swap)
}

void liberar_frame(int frame) {
    pthread_mutex_lock(&mutex_frames);
    if (frame >= 0 && frame < cantidad_marcos) {
        bitarray_clean_bit(frames_bitmap, frame);
    }
    pthread_mutex_unlock(&mutex_frames);
}

bool frame_esta_libre(int frame) {
    if (frame < 0 || frame >= cantidad_marcos) return false;
    return !bitarray_test_bit(frames_bitmap, frame);
}

int frames_totales(void) { return cantidad_marcos; }

int frames_ocupados(void) {
    pthread_mutex_lock(&mutex_frames);
    int ocupados = 0;
    for (int i = 0; i < cantidad_marcos; i++) {
        if (bitarray_test_bit(frames_bitmap, i))
            ocupados++;
    }
    pthread_mutex_unlock(&mutex_frames);
    return ocupados;
}
