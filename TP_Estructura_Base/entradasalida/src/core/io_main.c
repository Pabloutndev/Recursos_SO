
#include "io_main.h"
#include <interfaces/generic.h>
#include <interfaces/stdin.h>
#include <interfaces/dialfs.h>

t_log* logger;
t_io_config* config;
int socket_kernel;
int socket_memoria;
char* io_name;

#include <protocolo/op_code.h>

void conectar_modulos() {
    // Conectar Kernel
    socket_kernel = crear_conexion(config->ip_kernel, config->puerto_kernel);
    if (socket_kernel == -1) {
        log_error(logger, "Error conectando a Kernel");
        exit(EXIT_FAILURE);
    }
    // Handshake Kernel
    t_paquete* p = paquete_create(OP_HANDSHAKE_IO);
    paquete_write_string(p, io_name);
    enviar_paquete(socket_kernel, p);
    paquete_destroy(p);

    // Esperar OK de Kernel
    int respuesta;
    if(recv(socket_kernel, &respuesta, sizeof(int), MSG_WAITALL) <= 0 || respuesta != OP_OK) {
         log_error(logger, "Handshake Kernel fallido");
         exit(EXIT_FAILURE);
    }
    
    // Conectar Memoria (si es necesario)
    if (config->tipo_interfaz != IO_TYPE_GENERICA) {
        socket_memoria = crear_conexion(config->ip_memoria, config->puerto_memoria);
        if (socket_memoria == -1) {
            log_error(logger, "Error conectando a Memoria");
        } else {
             t_paquete* pm = paquete_create(OP_HANDSHAKE_IO);
             paquete_write_string(pm, io_name);
             enviar_paquete(socket_memoria, pm);
             paquete_destroy(pm);

             // Esperar OK de Memoria
             int resp_mem;
             recv(socket_memoria, &resp_mem, sizeof(int), MSG_WAITALL);
             // No validamos estricto para no romper si Memoria aun no implementa handshake io full
        }
    }
}

void io_loop() {
    while (1) {
        t_paquete* paquete = paquete_recv(socket_kernel);
        if (paquete == NULL) {
            log_error(logger, "Kernel desconectado");
            break;
        }

        switch (paquete->codigo_operacion) {
            case OP_IO_GENERIC_SLEEP:
                // [PID, TIEMPO]
                // Ejemplo deserializacion manual:
                // uint32_t pid; paquete_read_uint32(paquete, &pid);
                // uint32_t ms; paquete_read_uint32(paquete, &ms);
                // io_generic_handler(pid, ms);
                
                // Nota: io_generic_handler deberia ser adaptada para recibir valores
                // o pasamos el paquete.
                // Por ahora asumimos que los handlers seran refactorizados
                // o hacemos un wrapper.
                // io_generic_handler_wrapper(paquete);
                log_info(logger, "Solicitud IO_GENERIC_SLEEP recibida");
                break;

            case OP_IO_STDIN:
                log_info(logger, "Solicitud IO_STDIN recibida");
                break;
            case OP_IO_STDOUT:
                log_info(logger, "Solicitud IO_STDOUT recibida");
                break;
            case OP_IO_FS_CREATE:
            case OP_IO_FS_DELETE:
            case OP_IO_FS_TRUNCATE:
            case OP_IO_FS_WRITE:
            case OP_IO_FS_READ:
                log_info(logger, "Solicitud DIALFS recibida: %d", paquete->codigo_operacion);
                break;
            default:
                log_warning(logger, "Operacion desconocida: %d", paquete->codigo_operacion);
                break;
        }
        
        paquete_destroy(paquete);
    }
}

void io_init(const char* config_path, char* name) {
    io_name = name;
    logger = log_create("entradasalida.log", name, 1, LOG_LEVEL_INFO);
    config = io_config_create(config_path);
    
    if (!config) {
        log_error(logger, "Error cargando config");
        exit(EXIT_FAILURE);
    }
    
    // Initialize specific interface systems
    if (config->tipo_interfaz == IO_TYPE_DIALFS) {
        // io_dialfs_init(config, logger);
    }

    conectar_modulos();
    log_info(logger, "IO %s Iniciado - Tipo: %d", name, config->tipo_interfaz);
    
    io_loop();
    
    io_config_destroy(config);
    log_destroy(logger);
}
