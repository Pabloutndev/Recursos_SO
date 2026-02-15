
#include "dialfs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>
#include <math.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>

// ============================================================================
// Funciones de bitmap / gestion de bloques
// ============================================================================

// Calcula la cantidad de bloques necesarios para un tamanio dado
int bloques_necesarios(uint32_t tamanio) {
    if (tamanio == 0) return 0;
    return (int)ceil((double)tamanio / block_size);
}

// Busca una secuencia contigua de 'count' bloques libres en el bitmap
// Retorna el indice del primer bloque, o -1 si no encontro
int find_contiguous_blocks(int count) {
    if (count <= 0) return 0;

    int consecutive = 0;
    int start = -1;

    for (int i = 0; i < block_count; i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            if (consecutive == 0) start = i;
            consecutive++;
            if (consecutive == count) return start;
        } else {
            consecutive = 0;
            start = -1;
        }
    }
    return -1;
}

// Marca bloques como usados en el bitmap
void mark_blocks_used(int start, int count) {
    for (int i = start; i < start + count; i++) {
        bitarray_set_bit(bitmap, i);
    }
}

// Marca bloques como libres en el bitmap
void mark_blocks_free(int start, int count) {
    for (int i = start; i < start + count; i++) {
        bitarray_clean_bit(bitmap, i);
    }
}

// Sincroniza los archivos mmap'd a disco
void sync_all(void) {
    if (bitmap_mmap) {
        int bitmap_bytes = (int)ceil((double)block_count / 8.0);
        msync(bitmap_mmap, bitmap_bytes, MS_SYNC);
    }
    if (bloques_mmap) {
        msync(bloques_mmap, block_count * block_size, MS_SYNC);
    }
}

// Compactacion: mueve todos los archivos para que queden contiguos desde el bloque 0
void compactar(void) {
    log_info(fs_logger, "FS: Compactando filesystem...");

    // 1. Leer todos los archivos de metadata
    char* metadata_dir = string_from_format("%s/metadata", path_base);
    DIR* dir = opendir(metadata_dir);
    if (!dir) {
        log_error(fs_logger, "FS: No se pudo abrir directorio de metadata para compactacion");
        free(metadata_dir);
        return;
    }

    // Construir lista de FCBs con archivos que tienen bloques asignados
    t_list* archivos = list_create();
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Solo procesar archivos .metadata
        char* ext = strstr(entry->d_name, ".metadata");
        if (!ext || strlen(ext) != strlen(".metadata")) continue;

        // Extraer nombre del archivo (sin .metadata)
        int name_len = ext - entry->d_name;
        char nombre[256];
        memset(nombre, 0, sizeof(nombre));
        strncpy(nombre, entry->d_name, name_len);

        t_dialfs_fcb* fcb = malloc(sizeof(t_dialfs_fcb));
        *fcb = load_fcb(nombre);

        // Solo incluir archivos con bloques asignados
        if (fcb->bloque_inicial != (uint32_t)-1 && fcb->tamanio_archivo > 0) {
            list_add(archivos, fcb);
        } else {
            free(fcb);
        }
    }
    closedir(dir);
    free(metadata_dir);

    // 2. Ordenar por bloque_inicial
    bool _sort_by_block(void* a, void* b) {
        t_dialfs_fcb* fa = (t_dialfs_fcb*)a;
        t_dialfs_fcb* fb = (t_dialfs_fcb*)b;
        return fa->bloque_inicial <= fb->bloque_inicial;
    }
    list_sort(archivos, (void*)_sort_by_block);

    // 3. Crear buffer temporal para los datos
    size_t total_size = (size_t)block_count * block_size;
    void* temp_buffer = malloc(total_size);
    if (!temp_buffer) {
        log_error(fs_logger, "FS: No se pudo asignar memoria para compactacion (%zu bytes)", total_size);
        list_destroy_and_destroy_elements(archivos, free);
        return;
    }
    memset(temp_buffer, 0, total_size);

    // 4. Copiar datos de cada archivo al buffer temporal de forma contigua
    int next_free_block = 0;
    for (int i = 0; i < list_size(archivos); i++) {
        t_dialfs_fcb* fcb = list_get(archivos, i);
        int file_blocks = bloques_necesarios(fcb->tamanio_archivo);

        // Copiar datos del archivo desde su posicion actual al buffer temporal
        void* src = (char*)bloques_mmap + (fcb->bloque_inicial * block_size);
        void* dst = (char*)temp_buffer + (next_free_block * block_size);
        memcpy(dst, src, file_blocks * block_size);

        // Actualizar el bloque inicial del FCB
        fcb->bloque_inicial = next_free_block;
        next_free_block += file_blocks;
    }

    // 5. Copiar el buffer temporal de vuelta al mmap
    memcpy(bloques_mmap, temp_buffer, total_size);
    free(temp_buffer);

    // 6. Resetear el bitmap
    for (int i = 0; i < block_count; i++) {
        bitarray_clean_bit(bitmap, i);
    }

    // 7. Marcar los bloques usados y actualizar metadata
    int current_block = 0;
    for (int i = 0; i < list_size(archivos); i++) {
        t_dialfs_fcb* fcb = list_get(archivos, i);
        int file_blocks = bloques_necesarios(fcb->tamanio_archivo);
        mark_blocks_used(current_block, file_blocks);
        save_fcb(fcb);
        current_block += file_blocks;
    }

    // 8. Sync
    sync_all();

    // 9. Limpiar
    list_destroy_and_destroy_elements(archivos, free);

    log_info(fs_logger, "FS: Compactacion finalizada");
}
