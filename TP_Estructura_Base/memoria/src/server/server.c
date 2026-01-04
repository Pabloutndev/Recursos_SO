#include "../mod_memoria.h"
#include <utils/conexion.h>
#include <utils/paquete.h>
#include <pthread.h>
#include <sys/socket.h>
#include "../gestion/paginas.h"
#include "../gestion/memoria_core.h"
#include "../frames/frames.h"

/* Asumimos op_code definidos en utils/shared.h o similar. 
   Como no tengo el shared.h a mano con los enums exactos, usaré ints genéricos 
   o macros locales que DEBEN coincidir con los de Kernel/CPU.
*/
/* Usamos op_code de utils/paquete.h */

static int server_socket = -1;

void* atender_cliente(void* arg);

int server_init(const char* port) {
    server_socket = iniciar_servidor(port);
    if (server_socket < 0) return -1;
    log_info(logger, "Servidor Memoria iniciado en puerto %s", port);
    return 0;
}

void server_listen_loop(void) {
    while (1) {
        int cliente_fd = esperar_cliente(server_socket);
        if (cliente_fd < 0) {
            log_error(logger, "Error al aceptar cliente");
            continue;
        } 
        
        log_info(logger, "Cliente conectado FD %d", cliente_fd);
        
        pthread_t hilo;
        int* fd_ptr = malloc(sizeof(int));
        *fd_ptr = cliente_fd;
        pthread_create(&hilo, NULL, atender_cliente, fd_ptr);
        pthread_detach(hilo);
    }
}

void server_shutdown(void) {
    //close(server_socket); // release port
}

/* Helpers de serialización rápida (Asumiendo utils) */
static void send_ok(int fd) {
    int ok = 1;
    send(fd, &ok, sizeof(int), 0);
}

void* atender_cliente(void* arg) {
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
                // Enviar tamaño pagina y entradas por tabla (si aplica)
                int tam_pag = get_tamanio_pagina();
                send(fd, &tam_pag, sizeof(int), 0);
                break;

            case HANDSHAKE_KERNEL:
                log_info(logger, "Handshake KERNEL recibido");
                send_ok(fd);
                break;

            case INIT_PROCESO:
                // Recibir PID y Size
                lista = recibir_paquete(fd);
                pid = *(uint32_t*)list_get(lista, 0);
                size = *(int*)list_get(lista, 1);
                
                log_info(logger, "Solicitud Creacion Proceso: %d (Size: %d)", pid, size);
                
                if (paginacion_crear_proceso(pid, size)) {
                    log_info(logger, "Proceso creado OK");
                    // Responder OK? Kernel suele esperar OK
                    // Usar logica de utils si existe enviar_mensaje("OK")
                    // Hack rápido:
                    char* msg = "OK"; 
                    send(fd, msg, strlen(msg)+1, 0); // O enviar codigo
                } else {
                    log_error(logger, "Fallo creacion proceso");
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
                // Recibir PID y PC
                lista = recibir_paquete(fd);
                pid = *(uint32_t*)list_get(lista, 0);
                uint32_t pc = *(uint32_t*)list_get(lista, 1);
                
                // Simular retardo
                if (memoria_config->retardo_respuesta > 0)
                    usleep(memoria_config->retardo_respuesta * 1000);

                // lógica real: Ir al buffer de memoria o archivo.
                // Como aun no cargamos instrucciones a memoria, MOCK response:
                char* instruccion = "EXIT"; 
                // TODO: Implementar carga real de instrucciones en proceso_crear
                
                enviar_mensaje(instruccion, fd);
                list_destroy_and_destroy_elements(lista, free);
                break;
            
            case ACCESO_TABLA:
                // Recibir PID y Pagina -> Devolver Frame
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
                        // PAGE FAULT logic
                        int nuevo_frame = obtener_frame_libre();
                        if (nuevo_frame != -1) {
                            entry->frame = nuevo_frame;
                            entry->presente = true;
                            frame = nuevo_frame;
                            log_info(logger, "Page Fault resuelto PID %d Pag %d -> Frame %d", pid, pagina, frame);
                        } else {
                            // TODO: Reemplazo (Swap)
                            log_error(logger, "Memoria LLENA. Fallo asignar frame.");
                        }
                    }
                }
                
                send(fd, &frame, sizeof(int), 0);
                list_destroy_and_destroy_elements(lista, free);
                break;

            default:
                log_warning(logger, "Operacion desconocida: %d", cod_op);
                break;
        }
    }

    close(fd);
    return NULL;
}
