
#include "generic.h"

void io_generic_handler(t_list* packet, t_io_config* config, t_log* logger) {
    // Packet: [PID, UNITS]
    if (list_size(packet) < 2) return;
    
    int* pid = list_get(packet, 0);
    int* units = list_get(packet, 1);
    
    log_info(logger, "PID: %d - Operacion Generica - Unidades: %d", *pid, *units);
    
    int delay = config->tiempo_unidad_trabajo * (*units);
    usleep(delay * 1000); // ms to us
    
    log_info(logger, "PID: %d - Finalizo Operacion Generica", *pid);
}
