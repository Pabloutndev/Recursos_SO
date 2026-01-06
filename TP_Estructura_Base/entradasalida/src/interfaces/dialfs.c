
#include "dialfs.h"
#include <stdio.h>
// #include "blocks.h" // Hypothetical block manager

t_bitarray* bitmap;
void* bloques_mapper;
int bloques_fd;

void io_dialfs_init(t_io_config* cfg, t_log* logger) {
    if (cfg->tipo_interfaz != IO_TYPE_DIALFS) return;
    
    log_info(logger, "Iniciando Systema de Archivos DialFS en: %s", cfg->path_base_dialfs);
    
    // 1. Open/Create Blocks File
    // 2. Open/Create Bitmap File
    // 3. Load Bitarray
}

void io_dialfs_handler(int op_code, t_list* packet, t_io_config* cfg, t_log* logger, int socket_memoria) {
    // Dispatch FS operations
    // Packet [PID, SUB_OP params...]
    int* pid = list_get(packet, 0);
    
    switch (op_code) {
        case IO_FS_CREATE:
            // [PID, FILENAME]
            log_info(logger, "PID: %d - Crear Archivo", *pid);
            break;
        case IO_FS_DELETE:
            log_info(logger, "PID: %d - Eliminar Archivo", *pid);
            break;
        case IO_FS_TRUNCATE:
            log_info(logger, "PID: %d - Truncar Archivo", *pid);
            break;
        case IO_FS_WRITE:
            log_info(logger, "PID: %d - Escribir Archivo", *pid);
            break;
        case IO_FS_READ:
             log_info(logger, "PID: %d - Leer Archivo", *pid);
             break;
    }
}
