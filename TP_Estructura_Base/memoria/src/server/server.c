#include <mod_memoria.h>
#include <paquete/paquete.h>
#include <gestion/paginas.h>
#include <gestion/memoria_core.h>
#include <frames/frames.h>
#include <common/memoria/memoria.h> // Includes requests/responses logic
#include <protocolo/memoria.h>      // Helpers
#include <server/server.h>

static int server_socket = -1;

void* memoria_client_handler(void* arg);

int server_init(const char* port) {
    server_socket = iniciar_servidor((char*)port);
    if (server_socket < 0) return -1;
    log_info(logger, "Servidor Memoria iniciado en puerto %s", port);
    return 0;
}

void server_listen_loop(void) {
    server_escuchar(logger, "MEMORIA", server_socket, memoria_client_handler);
}

void server_shutdown(void) {
    close(server_socket);
}

static void send_ok(int fd) {
    int ok = 1; 
    send(fd, &ok, sizeof(int), 0);
}

void* memoria_client_handler(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    while(1) {
        t_paquete* paquete = paquete_recv(fd);
        if (paquete == NULL) {
            log_warning(logger, "Cliente FD %d desconectado", fd);
            break;
        }

        switch (paquete->codigo_operacion) {
            
            // =============================================================
            // HANDSHAKES
            // =============================================================
            case OP_HANDSHAKE_CPU:
                log_info(logger, "Handshake CPU recibido");
                 // Enviar tamanio de pagina como respuesta simple
                int tam_pag = get_tamanio_pagina();
                send(fd, &tam_pag, sizeof(int), 0);
                break;

            case OP_HANDSHAKE_KERNEL:
                log_info(logger, "Handshake KERNEL recibido");
                send_ok(fd);
                break;

            case OP_HANDSHAKE_IO: {
                // El handshake de IO suele venir con su nombre
                char* io_name = paquete_read_string(paquete);
                log_info(logger, "Handshake IO recibido: %s", io_name ? io_name : "UNKNOWN");
                free(io_name);
                send_ok(fd);
                break;
            }

            // =============================================================
            // GESTION PROCESOS
            // =============================================================
            case OP_INIT_PROCESO: {
                t_mem_init_proceso* req = deserializar_mem_init_proceso(paquete);
                if(req) {
                    log_info(logger, "Solicitud Creacion Proceso: %d (Size: %d)", req->pid, req->tamanio);
                    int result = OP_FAIL;
                    if (paginacion_crear_proceso(req->pid, req->tamanio)) {
                        log_info(logger, "Proceso creado OK");
                        result = OP_OK; 
                    } else {
                        log_error(logger, "Fallo creacion proceso");
                    }
                    send(fd, &result, sizeof(int), 0);
                    free(req);
                }
                break;
            }

            case OP_FIN_PROCESO: {
                t_mem_fin_proceso* req = deserializar_mem_fin_proceso(paquete);
                if(req) {
                    log_info(logger, "Solicitud Fin Proceso: %d", req->pid);
                    paginacion_destruir_proceso(req->pid);
                    free(req);
                }
                break;
            }

            // =============================================================
            // ACCESOS
            // =============================================================
            case OP_ACCESO_TABLA: {
                t_mem_traducir_pagina* req = deserializar_mem_traducir_pagina(paquete);
                if(req) {
                   log_info(logger, "Traduccion solicitada PID: %d Pagina: %d", req->pid, req->pagina);
                   manejar_traduccion_pagina(req, fd); 
                   free(req);
                }
                break;
            }
            
            case OP_FETCH_INSTRUCCION: {
                 t_mem_fetch* req = deserializar_mem_fetch(paquete);
                 if(req) {
                     log_info(logger, "Fetch Instruccion PID: %d IP: %d", req->pid, req->pc);
                     
                     if (memoria_config->retardo_respuesta > 0)
                        usleep(memoria_config->retardo_respuesta * 1000);

                     // Mock respuesta: En un caso real busco en memoria
                     char* instruccion = "WAIT RECURSO"; 
                     
                     t_paquete* resp = paquete_create(OP_RESPUESTA_INSTRUCCION);
                     paquete_write_string(resp, instruccion);
                     enviar_paquete(fd, resp);
                     paquete_destroy(resp);
                     
                     free(req);
                 }
                 break;
            }
            
            case OP_LEER_MEMORIA: {
                t_mem_read* req = recibir_lectura_memoria(paquete);
                if(req) {
                   log_info(logger, "Lectura PID: %d DirFisica: %d Tam: %d", req->pid, req->direccion_fisica, req->tamanio);
                   
                   void* buffer = malloc(req->tamanio);
                   bool ok = leer_memoria_fisica(req->direccion_fisica, buffer, req->tamanio);
                   
                   t_mem_respuesta_lectura res = { .ok = ok, .data = buffer, .size = req->tamanio };
                   enviar_respuesta_lectura(fd, &res);
                   
                   free(buffer);
                   free(req);
                }
                break;
            }

            case OP_ESCRIBIR_MEMORIA: {
                 t_mem_write* req = recibir_escritura_memoria(paquete);
                 if(req) {
                     log_info(logger, "Escritura PID: %d DirFisica: %d Tam: %d", req->pid, req->direccion_fisica, req->tamanio);
                     
                     bool ok = escribir_memoria_fisica(req->direccion_fisica, req->buffer, req->tamanio);
                     int result = ok ? OP_OK : OP_FAIL;
                     
                     send(fd, &result, sizeof(int), 0); // O enviar paquete OP_RESPUESTA_ESCRITURA
                     
                     if (req->buffer) free(req->buffer);
                     free(req);
                 }
                 break;
            }

            default:
                log_warning(logger, "Operacion desconocida: %d", paquete->codigo_operacion);
                break;
        }
        
        paquete_destroy(paquete);
    }

    close(fd);

    return NULL;
}