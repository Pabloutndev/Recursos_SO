#include <mod_memoria.h>
#include <paquete/paquete.h>
#include <gestion/paginas.h>
#include <gestion/memoria_core.h>
#include <frames/frames.h>
#include <common/memoria/memoria.h>
#include <protocolo/mensajes.h>
#include <server/server.h>
#include <protocolo/op_code.h>

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
        t_paquete* paquete = recibir_paquete(fd);
        if (paquete == NULL) {
            log_warning(logger, "Cliente FD %d desconectado", fd);
            break;
        }

        switch (paquete->codigo_operacion) {
            
            // =============================================================
            // HANDSHAKES
            // =============================================================
             case OP_HANDSHAKE:
                log_info(logger, "Handshake CPU recibido");
                int tam_pag = get_tamanio_pagina();
                send(fd, &tam_pag, sizeof(int), 0);
                break;
            /*
            case OP_HANDSHAKE_CPU:
                log_info(logger, "Handshake CPU recibido");
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
            */

            // =============================================================
            // GESTION PROCESOS
            // =============================================================
            case OP_MEM_INIT_PROCESO: {
                // Usamos el wrapper del protocolo
                t_mem_init_proceso* req = recibir_init_proceso(paquete);

                if(req) {
                    log_info(logger, "Solicitud Creacion Proceso: %d (Size: %d)", req->pid, req->tamanio);
                    bool success = paginacion_crear_proceso(req->pid, req->tamanio);
                    
                    if (success) {
                        log_info(logger, "Proceso creado OK");
                        enviar_respuesta_ok(fd);
                    } else {
                        log_error(logger, "Fallo creacion proceso");
                        enviar_respuesta_fail(fd);
                    }
                    free(req);
                }
                break;
            }

            case OP_MEM_FIN_PROCESO: {
                t_mem_fin_proceso* req = recibir_fin_proceso(paquete);
                if(req) {
                    log_info(logger, "Solicitud Fin Proceso: %d", req->pid);
                    paginacion_destruir_proceso(req->pid);
                    // No suele esperar respuesta síncrona, pero si hiciera falta:
                    // enviar_respuesta_ok(fd);
                    free(req);
                }
                break;
            }

            // =============================================================
            // ACCESOS
            // =============================================================
            case OP_MEM_TRADUCIR_PAGINA: {
                t_mem_traducir* req = recibir_mem_traducir_pagina(paquete);
                if(req) {
                   log_info(logger, "Traduccion solicitada PID: %d Pagina: %d", req->pid, req->direccion_logica);
                   
                   // Lógica de traducción
                   int frame = -1;
                   t_pagina* pag = paginacion_obtener_entrada(req->pid, req->direccion_logica);
                   if (pag && pag->frame != -1) {
                       frame = pag->frame;
                   }

                   t_mem_respuesta_traduccion res;
                   res.ok = (frame != -1);
                   res.direccion_fisica = (frame != -1) ? frame : 0;
                   
                   enviar_respuesta_traduccion(fd, &res);
                   
                   free(req);
                }
                break;
            }
            
            case OP_MEM_FETCH_INSTRUCCION: {
                 t_mem_fetch* req = recibir_fetch(paquete);
                 if(req) {
                     log_info(logger, "Fetch Instruccion PID: %d IP: %d", req->pid, req->pc);
                     
                     if (memoria_config->retardo_respuesta > 0)
                        usleep(memoria_config->retardo_respuesta * 1000);

                     char* instruccion = paginacion_leer_instruccion(req->pid, req->pc);
                     
                     if (!instruccion) {
                         instruccion = strdup("EXIT"); 
                         log_error(logger, "Error Fetch: Pagina no disponible o invalida");
                     }

                     enviar_respuesta_instruccion(fd, instruccion);
                     
                     free(instruccion);
                     free(req);
                 }
                 break;
            }
            
            case OP_MEM_LEER: {
                t_mem_read* req = recibir_lectura_memoria(paquete);
                if(req) {
                   log_info(logger, "Lectura PID: %d DirFisica: %d Tam: %d", req->pid, req->direccion_logica, req->size);
                   
                   void* buffer = malloc(req->size);
                   bool ok = leer_memoria_fisica(req->direccion_logica, buffer, req->size);
                   
                   t_mem_respuesta_lectura res = { .ok = ok, .data = buffer, .size = req->size };
                   enviar_respuesta_lectura(fd, &res);
                   
                   free(buffer);
                   free(req);
                }
                break;
            }

            case OP_MEM_ESCRIBIR: {
                 t_mem_write* req = recibir_escritura_memoria(paquete);
                 if(req) {

                    bool ok = escribir_memoria_fisica(req->direccion_logica, req->buffer, req->size);
                    
                    ok ? enviar_respuesta_ok(fd) : enviar_respuesta_fail(fd);
                    
                    log_info(logger, "Escritura PID: %d DirFisica: %d Tam: %d", req->pid, req->direccion_logica, req->size);
                    
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