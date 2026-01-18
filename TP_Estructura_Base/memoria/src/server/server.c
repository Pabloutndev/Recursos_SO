#include <mod_memoria.h>
#include <server/server_mem.h>
#include <paquete/paquete.h>
#include <model/model.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <adaptadores/memoria_adapter.h>
#include <conexion/conexion.h>

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

            // =============================================================
            // GESTION PROCESOS
            // =============================================================
            case OP_MEM_INIT_PROCESO:
                memoria_adapter_atender_init_proceso(fd, paquete);
                break;

            case OP_MEM_FIN_PROCESO:
                memoria_adapter_atender_fin_proceso(fd, paquete);
                break;

            // =============================================================
            // ACCESOS
            // =============================================================
            case OP_MEM_TRADUCIR_PAGINA:
                memoria_adapter_atender_traducir_pagina(fd, paquete);
                break;
            
            case OP_MEM_FETCH_INSTRUCCION:
                memoria_adapter_atender_fetch_instruccion(fd, paquete);
                break;
            
            case OP_MEM_LEER:
                memoria_adapter_atender_leer(fd, paquete);
                break;

            case OP_MEM_ESCRIBIR:
                memoria_adapter_atender_escribir(fd, paquete);
                break;

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