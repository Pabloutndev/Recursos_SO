
#include <interfaces/stdin.h>

void io_stdout_handler(t_list* packet, t_io_config* config, t_log* logger, int socket_memoria) {
      if (list_size(packet) < 2) return;
    
    int* pid = list_get(packet, 0);
    int* size = list_get(packet, 1);
    // int* addr = list_get(packet, 2);

    log_info(logger, "PID: %d - STDOUT - Imprimir %d bytes", *pid, *size);

    // Request to Memory
    // t_paquete* req = crear_paquete(LEER_MEMORIA);
    // enviar...
    // char* data = recibir_buffer...
    
    // Mockup
    char* mock_data = "Hola desde Memoria (Mock)"; 
    
    printf("STDOUT Output: %s\n", mock_data);
    
    log_info(logger, "PID: %d - Fin STDOUT", *pid);
}
