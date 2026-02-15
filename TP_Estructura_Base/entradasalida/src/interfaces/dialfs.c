
#include "dialfs.h"
#include "dialfs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>

// ============================================================================
// Estado global del filesystem
// ============================================================================

t_bitarray* bitmap = NULL;
void* bloques_mmap = NULL;   // mmap'd bloques.dat
void* bitmap_mmap = NULL;    // mmap'd bitmap.dat
int bloques_fd = -1;
int bitmap_fd = -1;
char* path_base = NULL;
int block_size = 0;
int block_count = 0;
t_log* fs_logger = NULL;

// ============================================================================
// Funciones publicas
// ============================================================================

void io_dialfs_init(t_io_config* config, t_log* logger) {
    if (config->tipo_interfaz != IO_TYPE_DIALFS) return;

    fs_logger = logger;
    path_base = strdup(config->path_base_dialfs);
    block_size = config->block_size;
    block_count = config->block_count;

    log_info(fs_logger, "FS: Iniciando Sistema de Archivos DialFS en: %s", path_base);
    log_info(fs_logger, "FS: BLOCK_SIZE=%d, BLOCK_COUNT=%d", block_size, block_count);

    // 1. Crear directorio base si no existe
    mkdir(path_base, 0755);

    // 2. Crear/abrir bloques.dat
    char* bloques_path = string_from_format("%s/bloques.dat", path_base);
    size_t bloques_total_size = (size_t)block_count * block_size;

    bloques_fd = open(bloques_path, O_RDWR | O_CREAT, 0644);
    if (bloques_fd == -1) {
        log_error(fs_logger, "FS: No se pudo abrir/crear bloques.dat en %s", bloques_path);
        free(bloques_path);
        return;
    }
    ftruncate(bloques_fd, bloques_total_size);

    bloques_mmap = mmap(NULL, bloques_total_size, PROT_READ | PROT_WRITE, MAP_SHARED, bloques_fd, 0);
    if (bloques_mmap == MAP_FAILED) {
        log_error(fs_logger, "FS: Error al hacer mmap de bloques.dat");
        close(bloques_fd);
        bloques_fd = -1;
        bloques_mmap = NULL;
        free(bloques_path);
        return;
    }
    free(bloques_path);
    log_info(fs_logger, "FS: bloques.dat inicializado (%zu bytes)", bloques_total_size);

    // 3. Crear/abrir bitmap.dat
    int bitmap_bytes = (int)ceil((double)block_count / 8.0);
    char* bitmap_path = string_from_format("%s/bitmap.dat", path_base);

    bitmap_fd = open(bitmap_path, O_RDWR | O_CREAT, 0644);
    if (bitmap_fd == -1) {
        log_error(fs_logger, "FS: No se pudo abrir/crear bitmap.dat en %s", bitmap_path);
        free(bitmap_path);
        return;
    }
    ftruncate(bitmap_fd, bitmap_bytes);

    bitmap_mmap = mmap(NULL, bitmap_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);
    if (bitmap_mmap == MAP_FAILED) {
        log_error(fs_logger, "FS: Error al hacer mmap de bitmap.dat");
        close(bitmap_fd);
        bitmap_fd = -1;
        bitmap_mmap = NULL;
        free(bitmap_path);
        return;
    }
    free(bitmap_path);

    bitmap = bitarray_create_with_mode(bitmap_mmap, bitmap_bytes, LSB_FIRST);
    log_info(fs_logger, "FS: bitmap.dat inicializado (%d bytes, %d bloques)", bitmap_bytes, block_count);

    // 4. Crear directorio de metadata si no existe
    char* metadata_dir = string_from_format("%s/metadata", path_base);
    mkdir(metadata_dir, 0755);
    free(metadata_dir);

    log_info(fs_logger, "FS: Sistema de archivos listo");
}

void io_dialfs_destroy(void) {
    if (bloques_mmap && bloques_mmap != MAP_FAILED) {
        size_t bloques_total_size = (size_t)block_count * block_size;
        munmap(bloques_mmap, bloques_total_size);
        bloques_mmap = NULL;
    }
    if (bloques_fd != -1) {
        close(bloques_fd);
        bloques_fd = -1;
    }

    if (bitmap) {
        bitarray_destroy(bitmap);
        bitmap = NULL;
    }
    if (bitmap_mmap && bitmap_mmap != MAP_FAILED) {
        int bitmap_bytes = (int)ceil((double)block_count / 8.0);
        munmap(bitmap_mmap, bitmap_bytes);
        bitmap_mmap = NULL;
    }
    if (bitmap_fd != -1) {
        close(bitmap_fd);
        bitmap_fd = -1;
    }

    if (path_base) {
        free(path_base);
        path_base = NULL;
    }

    log_info(fs_logger, "FS: Sistema de archivos destruido");
}

bool io_dialfs_create(const char* nombre) {
    if (metadata_exists(nombre)) {
        log_warning(fs_logger, "FS: Archivo '%s' ya existe", nombre);
        return false;
    }

    // Crear FCB con bloque_inicial = -1 y tamanio = 0
    t_dialfs_fcb fcb;
    memset(&fcb, 0, sizeof(t_dialfs_fcb));
    strncpy(fcb.nombre, nombre, sizeof(fcb.nombre) - 1);
    fcb.bloque_inicial = (uint32_t)-1;
    fcb.tamanio_archivo = 0;

    save_fcb(&fcb);

    log_info(fs_logger, "FS: Archivo '%s' creado", nombre);
    return true;
}

bool io_dialfs_delete(const char* nombre) {
    if (!metadata_exists(nombre)) {
        log_warning(fs_logger, "FS: Archivo '%s' no existe, no se puede eliminar", nombre);
        return false;
    }

    t_dialfs_fcb fcb = load_fcb(nombre);

    // Liberar bloques si tiene alguno asignado
    if (fcb.bloque_inicial != (uint32_t)-1 && fcb.tamanio_archivo > 0) {
        int num_blocks = bloques_necesarios(fcb.tamanio_archivo);
        mark_blocks_free(fcb.bloque_inicial, num_blocks);
        sync_all();
    }

    // Eliminar archivo de metadata
    char* path = metadata_path(nombre);
    remove(path);
    free(path);

    log_info(fs_logger, "FS: Archivo '%s' eliminado", nombre);
    return true;
}

bool io_dialfs_truncate(const char* nombre, uint32_t nuevo_tamanio) {
    if (!metadata_exists(nombre)) {
        log_error(fs_logger, "FS: Archivo '%s' no existe, no se puede truncar", nombre);
        return false;
    }

    t_dialfs_fcb fcb = load_fcb(nombre);
    int current_blocks = bloques_necesarios(fcb.tamanio_archivo);
    int new_blocks = bloques_necesarios(nuevo_tamanio);

    log_info(fs_logger, "FS: Truncar '%s' de %u (%d bloques) a %u (%d bloques)",
             nombre, fcb.tamanio_archivo, current_blocks, nuevo_tamanio, new_blocks);

    if (new_blocks == current_blocks) {
        // Misma cantidad de bloques, solo actualizar tamanio
        fcb.tamanio_archivo = nuevo_tamanio;
        save_fcb(&fcb);
        return true;
    }

    if (new_blocks > current_blocks) {
        // CRECIMIENTO
        if (fcb.bloque_inicial == (uint32_t)-1 || current_blocks == 0) {
            // Archivo sin bloques, buscar espacio contiguo
            int start = find_contiguous_blocks(new_blocks);
            if (start == -1) {
                // No hay espacio contiguo, intentar compactar
                compactar();
                start = find_contiguous_blocks(new_blocks);
                if (start == -1) {
                    log_error(fs_logger, "FS: No hay espacio suficiente para '%s' (%d bloques)", nombre, new_blocks);
                    return false;
                }
                // Recargar FCB despues de compactacion (pudo haber cambiado)
                fcb = load_fcb(nombre);
            }
            mark_blocks_used(start, new_blocks);
            fcb.bloque_inicial = start;
        } else {
            // Archivo ya tiene bloques, intentar extender
            int extra_blocks = new_blocks - current_blocks;
            int extend_start = fcb.bloque_inicial + current_blocks;
            bool can_extend = true;

            // Verificar que los bloques siguientes esten libres
            for (int i = extend_start; i < extend_start + extra_blocks; i++) {
                if (i >= block_count || bitarray_test_bit(bitmap, i)) {
                    can_extend = false;
                    break;
                }
            }

            if (can_extend) {
                // Extender en el lugar
                mark_blocks_used(extend_start, extra_blocks);
            } else {
                // Buscar nuevo espacio contiguo
                int new_start = find_contiguous_blocks(new_blocks);
                if (new_start == -1) {
                    // Compactar y reintentar
                    compactar();
                    // Recargar FCB despues de compactacion
                    fcb = load_fcb(nombre);
                    current_blocks = bloques_necesarios(fcb.tamanio_archivo);

                    new_start = find_contiguous_blocks(new_blocks);
                    if (new_start == -1) {
                        log_error(fs_logger, "FS: No hay espacio suficiente para '%s' despues de compactar", nombre);
                        return false;
                    }
                }

                // Copiar datos del viejo lugar al nuevo
                if (fcb.bloque_inicial != (uint32_t)-1 && current_blocks > 0) {
                    void* src = (char*)bloques_mmap + (fcb.bloque_inicial * block_size);
                    void* dst = (char*)bloques_mmap + (new_start * block_size);
                    memmove(dst, src, current_blocks * block_size);

                    // Liberar bloques viejos
                    mark_blocks_free(fcb.bloque_inicial, current_blocks);
                }

                // Marcar nuevos bloques como usados
                mark_blocks_used(new_start, new_blocks);
                fcb.bloque_inicial = new_start;
            }
        }
    } else {
        // REDUCCION
        if (new_blocks == 0) {
            // Liberar todos los bloques
            if (fcb.bloque_inicial != (uint32_t)-1 && current_blocks > 0) {
                mark_blocks_free(fcb.bloque_inicial, current_blocks);
            }
            fcb.bloque_inicial = (uint32_t)-1;
        } else if (fcb.bloque_inicial != (uint32_t)-1 && current_blocks > 0) {
            // Liberar bloques del final
            int blocks_to_free = current_blocks - new_blocks;
            int free_start = fcb.bloque_inicial + new_blocks;
            mark_blocks_free(free_start, blocks_to_free);
        }
    }

    fcb.tamanio_archivo = nuevo_tamanio;
    save_fcb(&fcb);
    sync_all();

    log_info(fs_logger, "FS: Archivo '%s' truncado a %u bytes (bloque_inicial: %d, bloques: %d)",
             nombre, nuevo_tamanio, (int)fcb.bloque_inicial, new_blocks);
    return true;
}

bool io_dialfs_write(const char* nombre, const void* data, uint32_t size, uint32_t offset_archivo) {
    if (!metadata_exists(nombre)) {
        log_error(fs_logger, "FS: Archivo '%s' no existe, no se puede escribir", nombre);
        return false;
    }

    t_dialfs_fcb fcb = load_fcb(nombre);

    if (fcb.bloque_inicial == (uint32_t)-1) {
        log_error(fs_logger, "FS: Archivo '%s' no tiene bloques asignados", nombre);
        return false;
    }

    if (offset_archivo + size > fcb.tamanio_archivo) {
        log_error(fs_logger, "FS: Write fuera de rango para '%s' (offset=%u, size=%u, tamanio=%u)",
                  nombre, offset_archivo, size, fcb.tamanio_archivo);
        return false;
    }

    // Calcular posicion en el mmap
    size_t pos = (size_t)(fcb.bloque_inicial * block_size) + offset_archivo;
    memcpy((char*)bloques_mmap + pos, data, size);

    sync_all();

    log_info(fs_logger, "FS: Escritos %u bytes en '%s' (offset=%u)", size, nombre, offset_archivo);
    return true;
}

bool io_dialfs_read(const char* nombre, void* buffer, uint32_t size, uint32_t offset_archivo) {
    if (!metadata_exists(nombre)) {
        log_error(fs_logger, "FS: Archivo '%s' no existe, no se puede leer", nombre);
        return false;
    }

    t_dialfs_fcb fcb = load_fcb(nombre);

    if (fcb.bloque_inicial == (uint32_t)-1) {
        log_error(fs_logger, "FS: Archivo '%s' no tiene bloques asignados", nombre);
        return false;
    }

    if (offset_archivo + size > fcb.tamanio_archivo) {
        log_error(fs_logger, "FS: Read fuera de rango para '%s' (offset=%u, size=%u, tamanio=%u)",
                  nombre, offset_archivo, size, fcb.tamanio_archivo);
        return false;
    }

    // Calcular posicion en el mmap
    size_t pos = (size_t)(fcb.bloque_inicial * block_size) + offset_archivo;
    memcpy(buffer, (char*)bloques_mmap + pos, size);

    log_info(fs_logger, "FS: Leidos %u bytes de '%s' (offset=%u)", size, nombre, offset_archivo);
    return true;
}
