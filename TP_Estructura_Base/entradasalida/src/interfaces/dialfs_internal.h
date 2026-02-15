
#ifndef INTERFACES_DIALFS_INTERNAL_H_
#define INTERFACES_DIALFS_INTERNAL_H_

#include "dialfs.h"
#include <commons/bitarray.h>
#include <commons/log.h>

// ============================================================================
// Estado global del filesystem (definido en dialfs.c, compartido internamente)
// ============================================================================

extern t_bitarray* bitmap;
extern void* bloques_mmap;
extern void* bitmap_mmap;
extern int bloques_fd;
extern int bitmap_fd;
extern char* path_base;
extern int block_size;
extern int block_count;
extern t_log* fs_logger;

// ============================================================================
// Funciones de bitmap / gestion de bloques (dialfs_bitmap.c)
// ============================================================================

int bloques_necesarios(uint32_t tamanio);
int find_contiguous_blocks(int count);
void mark_blocks_used(int start, int count);
void mark_blocks_free(int start, int count);
void sync_all(void);
void compactar(void);

// ============================================================================
// Funciones de metadata / FCB (dialfs_metadata.c)
// ============================================================================

char* metadata_path(const char* nombre);
t_dialfs_fcb load_fcb(const char* nombre);
void save_fcb(const t_dialfs_fcb* fcb);
bool metadata_exists(const char* nombre);

#endif
