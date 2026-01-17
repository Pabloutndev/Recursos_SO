#include <mod_memoria.h>
#include <server/server_mem.h>
#include <paquete/paquete.h>
#include <common/memoria/memoria.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <adaptadores/memoria_adapter.h>
#include <server/server.h>

static int server_socket = -1;

int server_init(char* port)
{
    server_socket = iniciar_servidor(port);
    if (server_socket < 0) return -1;
    log_info(logger, "Servidor Memoria iniciado en puerto %s", port);
    return 0;
}

void server_listen_loop(void)
{
    server_escuchar(logger, "MEMORIA", server_socket, memoria_client_handler);
}

void server_shutdown(void)
{
    close(server_socket);
}

void* memoria_client_handler(void* arg)
{
    int fd = *(int*)arg;
    free(arg);

    log_info(logger, "Cliente conectado (FD=%d)", fd);

    while (1) {
        t_paquete* paquete = recibir_paquete(fd);
        if (paquete == NULL) {
            log_warning(logger, "Cliente FD %d desconectado", fd);
            break;
        }

        log_info(logger, "Servidor recibió: OP_CODE=%d", paquete->codigo_operacion);

        switch (paquete->codigo_operacion) {

        // ========================================
        // HANDSHAKE
        // ========================================
        case OP_HANDSHAKE: {
            log_info(logger, "Handshake recibido");
            int tam_pag = get_tamanio_pagina();
            send(fd, &tam_pag, sizeof(int), 0);
            break;
        }

        // ========================================
        // GESTIÓN DE PROCESOS (Kernel)
        // ========================================
        case OP_MEM_INIT_PROCESO: {
            log_info(logger, "Servidor: OP_MEM_INIT_PROCESO");
            t_mem_init_proceso* req = recibir_init_proceso(paquete);
            if (req) {
                // DELEGA AL ADAPTADOR
                memoria_adapter_init_proceso(req, fd);
                free(req);
            }
            break;
        }

        case OP_MEM_FIN_PROCESO: {
            log_info(logger, "Servidor: OP_MEM_FIN_PROCESO");
            t_mem_fin_proceso* req = recibir_fin_proceso(paquete);
            if (req) {
                // DELEGA AL ADAPTADOR
                memoria_adapter_fin_proceso(req, fd);
                free(req);
            }
            break;
        }

        // ========================================
        // ACCESO A MEMORIA (CPU)
        // ========================================
        case OP_MEM_TRADUCIR_PAGINA: {
            log_info(logger, "Servidor: OP_MEM_TRADUCIR_PAGINA");
            t_mem_traducir* req = recibir_mem_traducir_pagina(paquete);
            if (req) {
                // DELEGA AL ADAPTADOR
                memoria_adapter_traducir_pagina(req, fd);
                free(req);
            }
            break;
        }

        case OP_MEM_FETCH_INSTRUCCION: {
            log_info(logger, "Servidor: OP_MEM_FETCH_INSTRUCCION");
            t_mem_fetch* req = recibir_fetch(paquete);
            if (req) {
                // DELEGA AL ADAPTADOR
                memoria_adapter_fetch_instruccion(req, fd);
                free(req);
            }
            break;
        }

        case OP_MEM_LEER: {
            log_info(logger, "Servidor: OP_MEM_LEER");
            t_mem_read* req = recibir_lectura_memoria(paquete);
            if (req) {
                // DELEGA AL ADAPTADOR
                memoria_adapter_leer(req, fd);
                free(req);
            }
            break;
        }

        case OP_MEM_ESCRIBIR: {
            log_info(logger, "Servidor: OP_MEM_ESCRIBIR");
            t_mem_write* req = recibir_escritura_memoria(paquete);
            if (req) {
                // DELEGA AL ADAPTADOR
                memoria_adapter_escribir(req, fd);
                if (req->buffer) free(req->buffer);
                free(req);
            }
            break;
        }

        default:
            log_warning(logger, "Operacion desconocida: %d", 
                       paquete->codigo_operacion);
            break;
        }

        paquete_destroy(paquete);
    }

    close(fd);
    return NULL;
}

        paquete_destroy(paquete);
    }

    close(fd);

    return NULL;
}