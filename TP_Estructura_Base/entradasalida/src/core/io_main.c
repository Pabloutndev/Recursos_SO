
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
#include <protocolo/mensajes.h>
#include <paquete/paquete.h>

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

    // Esperar OK de Kernel (paquete con OP_OK/OP_FAIL)
    t_paquete* resp_kernel = recibir_paquete(socket_kernel);
    if (!resp_kernel || !recibir_respuesta(resp_kernel)) {
         log_error(logger, "Handshake Kernel fallido");
         exit(EXIT_FAILURE);
    }
    paquete_destroy(resp_kernel);
    
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

             // Esperar OK de Memoria (paquete)
             t_paquete* resp_mem = recibir_paquete(socket_memoria);
             if (resp_mem) paquete_destroy(resp_mem);
             // No validamos estricto para no romper si Memoria aun no implementa handshake io full
        }
    }
}

void io_loop() {
    while (1) {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        if (paquete == NULL) {
            log_error(logger, "Kernel desconectado");
            break;
        }

        switch (paquete->codigo_operacion) {
            case OP_IO_GENERIC_SLEEP: {
                log_info(logger, "Solicitud IO_GENERIC_SLEEP recibida");
                // Desserializar PID y tiempo
                uint32_t pid;
                uint32_t tiempo_ms;
                paquete_read_uint32(paquete, &pid);
                paquete_read_uint32(paquete, &tiempo_ms);
                
                // Ejecutar sleep
                log_info(logger, "IO_SLEEP PID %u por %u ms", pid, tiempo_ms);
                usleep(tiempo_ms * 1000);  // Convertir ms a microsegundos
                
                // Responder al Kernel con OP_IO_FIN_OPERACION
                enviar_respuesta_ok(socket_kernel);
                break;
            }

            case OP_IO_STDIN_READ: {
                log_info(logger, "Solicitud IO_STDIN_READ recibida");
                uint32_t pid;
                paquete_read_uint32(paquete, &pid);
                
                // Ejecutar lectura desde stdin
                log_info(logger, "IO_STDIN_READ PID %u", pid);
                // TODO: Implementar lectura real desde stdin
                
                // Responder al Kernel
                enviar_respuesta_ok(socket_kernel);
                break;
            }

            case OP_IO_STDOUT_WRITE: {
                log_info(logger, "Solicitud IO_STDOUT_WRITE recibida");
                uint32_t pid;
                paquete_read_uint32(paquete, &pid);
                
                // Ejecutar escritura a stdout
                log_info(logger, "IO_STDOUT_WRITE PID %u", pid);
                // TODO: Implementar escritura real a stdout
                
                // Responder al Kernel
                enviar_respuesta_ok(socket_kernel);
                break;
            }

            case OP_IO_FS_CREATE:
            case OP_IO_FS_DELETE:
            case OP_IO_FS_TRUNCATE:
            case OP_IO_FS_WRITE:
            case OP_IO_FS_READ: {
                log_info(logger, "Solicitud DIALFS recibida: %d", paquete->codigo_operacion);
                uint32_t pid;
                paquete_read_uint32(paquete, &pid);
                
                // Ejecutar operación FS según código
                // TODO: Delegar a interfaz dialfs/generic según tipo_interfaz
                log_info(logger, "IO_FS_OP PID %u - Code %d", pid, paquete->codigo_operacion);
                
                // Responder al Kernel
                enviar_respuesta_ok(socket_kernel);
                break;
            }

            default:
                log_warning(logger, "Operacion desconocida: %d", paquete->codigo_operacion);
                enviar_respuesta_fail(socket_kernel);
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
