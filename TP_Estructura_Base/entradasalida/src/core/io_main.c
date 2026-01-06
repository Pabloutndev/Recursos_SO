
#include "io_main.h"
#include "../interfaces/generic.h"
#include "../interfaces/stdin.h"
#include "../interfaces/stdout.h"
#include "../interfaces/dialfs.h"

t_log* logger;
t_io_config* config;
int socket_kernel;
int socket_memoria;
char* io_name;

void conectar_modulos() {
    // Conectar Kernel
    socket_kernel = crear_conexion(config->ip_kernel, config->puerto_kernel);
    if (socket_kernel == -1) {
        log_error(logger, "Error conectando a Kernel");
        exit(EXIT_FAILURE);
    }
    // Handshake Kernel
    // Enviamos nombre o ID
    t_paquete* p = crear_paquete(HANDSHAKE_IO);
    agregar_a_paquete(p, io_name, strlen(io_name) + 1);
    enviar_paquete(p, socket_kernel);
    eliminar_paquete(p);
    
    // Conectar Memoria (si es necesario por tipo de interfaz, pero lo hacemos siempre para simplificar por ahora)
    if (config->tipo_interfaz != IO_GENERICA) {
        socket_memoria = crear_conexion(config->ip_memoria, config->puerto_memoria);
        if (socket_memoria == -1) {
            log_error(logger, "Error conectando a Memoria");
            // No fatal si es generica, pero ya filtramos
        } else {
             t_paquete* pm = crear_paquete(HANDSHAKE_IO);
             agregar_a_paquete(pm, io_name, strlen(io_name) + 1);
             enviar_paquete(pm, socket_memoria);
             eliminar_paquete(pm);
        }
    }
}

void io_loop() {
    while (1) {
        int op_code = recibir_operacion(socket_kernel);
        if (op_code < 0) {
            log_error(logger, "Kernel desconectado");
            break;
        }

        t_list* packet = recibir_paquete(socket_kernel);
        // [PID, ...] params logic depends on op_code
        // Assumes first element is always PID (int)
        
        switch (op_code) {
            case IO_GENERIC_SLEEP:
                io_generic_handler(packet, config, logger);
                break;
            case IO_STDIN:
                io_stdin_handler(packet, config, logger, socket_memoria);
                break;
            case IO_STDOUT:
                io_stdout_handler(packet, config, logger, socket_memoria);
                break;
            case IO_FS_CREATE:
            case IO_FS_DELETE:
            case IO_FS_TRUNCATE:
            case IO_FS_WRITE:
            case IO_FS_READ:
                io_dialfs_handler(op_code, packet, config, logger, socket_memoria);
                break;
            default:
                log_warning(logger, "Operacion desconocida: %d", op_code);
                break;
        }
        
        // Notify Kernel Finish (Generic simple implementations often block kernel until done)
        // Usually we send PID back + Status
        t_paquete* resp = crear_paquete(FIN_DE_PROCESO); // Or specific IO_FIN
        // Using generic response for now
        enviar_codigo(socket_kernel, 10); // OK or FIN
        // Note: Real implementation needs strict protocol compliance.
        
        list_destroy_and_destroy_elements(packet, free);
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
    if (config->tipo_interfaz == IO_DIALFS) {
        io_dialfs_init(config, logger);
    }

    conectar_modulos();
    log_info(logger, "IO %s Iniciado - Tipo: %d", name, config->tipo_interfaz);
    
    io_loop();
    
    io_config_destroy(config);
    log_destroy(logger);
}
