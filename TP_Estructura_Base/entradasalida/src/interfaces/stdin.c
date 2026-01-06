
#include "stdin.h"

void io_stdin_handler(t_list* packet, t_io_config* config, t_log* logger, int socket_memoria) {
    if (list_size(packet) < 2) return;
    
    // Suponemos packet: [PID, SIZE, REGS...] - Simplificado: [PID, SIZE, ADDR] for now
    // Or just [PID] and we prompt user.
    // Spec usually says: "Wait for input, then write to memory".
    
    int* pid = list_get(packet, 0);
    int* size = list_get(packet, 1); // Size to read
    // int* addr = list_get(packet, 2); // Phys Addr
    
    log_info(logger, "PID: %d - STDIN - Solicita %d bytes", *pid, *size);
    
    printf("Ingrese texto (max %d bytes): ", *size);
    char* input = readline("> ");
    
    if (input) {
         // Truncate to size
         if (strlen(input) > *size) input[*size] = '\0';
         
         // Send to Memory
         // Construct packet: [PID][ADDR][CONTENT]... this requires loops if fragmented
         // For now, assume simplified 1 segment logic or just log successful read.
         
         log_info(logger, "Leido: %s", input);
         
         // Actual memory write (Mockup)
         // t_paquete* p = crear_paquete(ESCRIBIR_MEMORIA);
         // agregar_entero(p, *addr);
         // agregar_buffer(p, input, strlen(input)+1);
         // enviar_paquete(p, socket_memoria);
         // esperar_confirmacion(socket_memoria);
         
         free(input);
    }
    
    log_info(logger, "PID: %d - Fin STDIN", *pid);
}
