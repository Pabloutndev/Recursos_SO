#include "io_main.h"
#include <interfaces/generic.h>
#include <interfaces/stdin.h>
#include <interfaces/dialfs.h>

#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <paquete/paquete.h>
#include <conexion/conexion.h>
#include <commons/string.h>
#include <stdlib.h>

t_io_context IO_CTX;

void conectar_modulos() {

    // =============================================
    // Seccion 1: Conexion y handshake con Kernel
    // =============================================
    IO_CTX.socket_kernel = crear_conexion(IO_CTX.config->ip_kernel, IO_CTX.config->puerto_kernel);
    if (IO_CTX.socket_kernel == -1) {
        log_error(IO_CTX.logger, "Error conectando a Kernel");
        exit(EXIT_FAILURE);
    }

    t_paquete* p = paquete_create(OP_HANDSHAKE_IO);
    paquete_write_string(p, IO_CTX.io_name);
    enviar_paquete(IO_CTX.socket_kernel, p);
    paquete_destroy(p);

    t_paquete* resp_kernel = recibir_paquete(IO_CTX.socket_kernel);
    if (!resp_kernel || !recibir_respuesta(resp_kernel)) {
         log_error(IO_CTX.logger, "Handshake Kernel fallido");
         exit(EXIT_FAILURE);
    }
    paquete_destroy(resp_kernel);

    // =============================================
    // Seccion 2: Conexion y handshake con Memoria
    // Solo se conecta si la interfaz no es GENERICA
    // (STDIN, STDOUT y DIALFS necesitan acceso a memoria)
    // =============================================
    if (IO_CTX.config->tipo_interfaz != IO_TYPE_GENERICA) {
        IO_CTX.socket_memoria = crear_conexion(IO_CTX.config->ip_memoria, IO_CTX.config->puerto_memoria);
        if (IO_CTX.socket_memoria == -1) {
            log_error(IO_CTX.logger, "Error conectando a Memoria (requerida para %s)", IO_CTX.io_name);
            exit(EXIT_FAILURE);
        }

        t_paquete* pm = paquete_create(OP_HANDSHAKE_IO);
        paquete_write_string(pm, IO_CTX.io_name);
        enviar_paquete(IO_CTX.socket_memoria, pm);
        paquete_destroy(pm);

        t_paquete* resp_mem = recibir_paquete(IO_CTX.socket_memoria);
        if (resp_mem) paquete_destroy(resp_mem);
    }
}

void io_init(const char* config_path, char* name) {
    IO_CTX.io_name = name;
    
    char* log_filename = string_from_format("%s.log", name);
    IO_CTX.logger = log_create(log_filename, name, 1, LOG_LEVEL_INFO);
    free(log_filename);
    
    IO_CTX.config = io_config_create(config_path);
    
    if (!IO_CTX.config) {
        log_error(IO_CTX.logger, "Error cargando config");
        exit(EXIT_FAILURE);
    }
    
    // Initialize specific interface systems
    if (IO_CTX.config->tipo_interfaz == IO_TYPE_DIALFS) {
        io_dialfs_init(IO_CTX.config, IO_CTX.logger);
    }

    conectar_modulos();
    log_info(IO_CTX.logger, "IO %s Iniciado - Tipo: %d", name, IO_CTX.config->tipo_interfaz);
    
    io_receiver_loop();
    
    io_config_destroy(IO_CTX.config);
    log_destroy(IO_CTX.logger);
}
