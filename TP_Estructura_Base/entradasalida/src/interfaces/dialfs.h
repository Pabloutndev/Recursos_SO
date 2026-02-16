
#ifndef INTERFACES_DIALFS_H_
#define INTERFACES_DIALFS_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <config/io_config.h>
#include <paquete/paquete.h>
#include <stdbool.h>

// Metadata de un archivo en DIALFS
typedef struct {
    char nombre[256];
    uint32_t bloque_inicial;
    uint32_t tamanio_archivo;
} t_dialfs_fcb;

void io_dialfs_init(t_io_config* config, t_log* logger);
void io_dialfs_destroy(void);

bool io_dialfs_create(const char* nombre);
bool io_dialfs_delete(const char* nombre);
bool io_dialfs_truncate(const char* nombre, uint32_t nuevo_tamanio);

// Write: lee datos de buffer y escribe en el archivo a partir de offset
bool io_dialfs_write(const char* nombre, const void* data, uint32_t size, uint32_t offset_archivo);

// Read: lee datos del archivo y los pone en buffer
bool io_dialfs_read(const char* nombre, void* buffer, uint32_t size, uint32_t offset_archivo);

#endif
