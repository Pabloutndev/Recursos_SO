#include <mod_memoria.h>
#include <paquete/paquete.h>
#include <gestion/paginas.h>
#include <gestion/memoria_core.h>
#include <frames/frames.h>
#include <server/server.h> // Generic server

static int server_socket = -1;

// Forward declaration of the handler
void* memoria_client_handler(void* arg);

int server_init(const char* port) {
    server_socket = iniciar_servidor((char*)port);
    if (server_socket < 0) return -1;
    log_info(logger, "Servidor Memoria iniciado en puerto %s", port);
    return 0;
}

void server_listen_loop(void) {
    // Calls generic server loop with specific handler
    server_escuchar(logger, "MEMORIA", server_socket, memoria_client_handler);
}

void server_shutdown(void) {
    close(server_socket);
}

// Helper to send OK
static void send_ok(int fd) {
    int ok = 1; // OK
    send(fd, &ok, sizeof(int), 0);
}

void* memoria_client_handler(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    while(1) {
        int cod_op = recibir_operacion(fd);
        if (cod_op < 0) {
            log_warning(logger, "Cliente FD %d desconectado", fd);
            break;
        }

        t_list* lista;
        uint32_t pid;
        int size;
        
        switch (cod_op) {
            case HANDSHAKE_CPU:
                log_info(logger, "Handshake CPU recibido");
                int tam_pag = get_tamanio_pagina();
                send(fd, &tam_pag, sizeof(int), 0);
                break;

            case HANDSHAKE_KERNEL:
                log_info(logger, "Handshake KERNEL recibido");
                send_ok(fd);
                break;
            case HANDSHAKE_IO:
                 // Consume payload (Name)
                 lista = recibir_paquete(fd);
                 char* io_name = (char*)list_get(lista, 0); // Assuming first item is string
                 log_info(logger, "Handshake IO recibido: %s", io_name);
                 
                 // If IO module logic requires storing connection, do it here.
                 // For now just ack.
                 send_ok(fd);
                 list_destroy_and_destroy_elements(lista, free);
                 break;

            case INIT_PROCESO:
                lista = recibir_paquete(fd);
                pid = *(uint32_t*)list_get(lista, 0);
                size = *(int*)list_get(lista, 1);
                
                log_info(logger, "Solicitud Creacion Proceso: %d (Size: %d)", pid, size);
                
                if (paginacion_crear_proceso(pid, size)) {
                    log_info(logger, "Proceso creado OK");
                    // Assuming kernel waits for int code or msg
                    int resp = OK; 
                    send(fd, &resp, sizeof(int), 0);
                } else {
                    log_error(logger, "Fallo creacion proceso");
                     int resp = FAIL; 
                    send(fd, &resp, sizeof(int), 0);
                }
                list_destroy_and_destroy_elements(lista, free);
                break;

            case FIN_PROCESO:
                lista = recibir_paquete(fd);
                pid = *(uint32_t*)list_get(lista, 0);
                log_info(logger, "Solicitud Fin Proceso: %d", pid);
                paginacion_destruir_proceso(pid);
                list_destroy_and_destroy_elements(lista, free);
                break;

            case FETCH_INSTRUCCION:
                lista = recibir_paquete(fd);
                pid = *(uint32_t*)list_get(lista, 0);
                uint32_t pc = *(uint32_t*)list_get(lista, 1);
                (void)pc; // Fix unused variable warning
                
                if (memoria_config->retardo_respuesta > 0)
                    usleep(memoria_config->retardo_respuesta * 1000);

                char* instruccion = "EXIT"; // Mock
                enviar_mensaje(instruccion, fd);
                list_destroy_and_destroy_elements(lista, free);
                break;
            
            case ACCESO_TABLA:
                lista = recibir_paquete(fd);
                pid = *(uint32_t*)list_get(lista, 0);
                int pagina = *(int*)list_get(lista, 1);
                
                if (memoria_config->retardo_respuesta > 0)
                    usleep(memoria_config->retardo_respuesta * 1000);

                t_pagina* entry = paginacion_obtener_entrada(pid, pagina);
                int frame = -1;
                if (entry) {
                    if (entry->presente) {
                        frame = entry->frame;
                        entry->uso = true;
                    } else {
                        // Page Fault
                         frame = -1; // Indicate Page Fault to requester logic? 
                         // Or handle loading here? Usually Memory replies frame index.
                         // If -1, CPU/Kernel handles page fault logic or Memory blocks.
                         // Simple approach: Return -1.
                    }
                }
                
                send(fd, &frame, sizeof(int), 0);
                list_destroy_and_destroy_elements(lista, free);
                break;
            
            // IO Handlers (Leer/Escribir)
            case LEER_MEMORIA:
                 // [PID, ADDR, SIZE]
                 // ... implementation
                 break;
            case ESCRIBIR_MEMORIA:
                 // [PID, ADDR, CONTENT]
                 // ... implementation
                 break;

            default:
                log_warning(logger, "Operacion desconocida: %d", cod_op);
                break;
        }
    }

    close(fd);
    return NULL;
}
