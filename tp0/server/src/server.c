#include "server.h"
#include <pthread.h>

t_log* logger;

int main(void) {
    logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    int server_fd = iniciar_servidor();

    while (1) {
        int cliente_fd = esperar_cliente(server_fd);

        int* fd_para_hilo = malloc(sizeof(int));
        *fd_para_hilo = cliente_fd;

        pthread_t hilo;
        pthread_create(&hilo, NULL, atender_cliente, fd_para_hilo);
        pthread_detach(hilo);  // No hace falta join
    }
}

void* atender_cliente(void* arg) 
{
    int cliente_fd = *(int*)arg;
    free(arg);

    log_info(logger, "Hilo atendiendo cliente %d", cliente_fd);

    while (1) {
        int cod_op = recibir_operacion(cliente_fd);

        if (cod_op == -1) {
            log_warning(logger, "Cliente %d desconectado", cliente_fd);
            close(cliente_fd);
            return NULL;
        }

        switch (cod_op) 
		{
			case MENSAJE:
				recibir_mensaje(cliente_fd);
				break;
			case PAQUETE: 
			{
				t_list* lista = recibir_paquete(cliente_fd);
				list_iterate(lista, (void*)iterator);
				list_destroy_and_destroy_elements(lista, free);
				break;
        	}
			default:
				log_warning(logger, "Operacion desconocida");
				break;
        }
    }
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}
