// consola.c - Modulo Consola Standalone
#include <consola.h>
#include <config/consola_config.h>
#include <helpers/consola_helpers.h>
#include <adaptadores/consola_kernel_adapter.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <commons/log.h>
#include <commons/string.h>

#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>

// Estado global de la consola
static t_log* logger_consola = NULL;
static t_consola_config config_consola;
static int socket_kernel = -1;

// ============================
// Procesamiento de linea
// ============================

static bool procesar_linea(char* linea)
{
    char** tokens = string_split(linea, " ");

    if (!tokens[0]) {
        string_iterate_lines(tokens, (void(*)(char*)) free);
        free(tokens);
        return false;
    }

    comando_t cmd = obtener_comando(tokens[0]);

    switch (cmd) {

        case CMD_RUN:
            if (tokens[1]) {
                uint32_t prioridad = 0;
                if (tokens[2]) prioridad = (uint32_t)atoi(tokens[2]);
                enviar_comando_run(socket_kernel, tokens[1], prioridad);
                log_info(logger_consola, "Enviado RUN: %s (prioridad=%u)", tokens[1], prioridad);
            } else {
                log_error(logger_consola, "RUN requiere un path");
            }
            break;

        case CMD_KILL: {
            int pid = obtener_pid(tokens[1]);
            if (pid == -1) {
                log_error(logger_consola, "KILL requiere un PID valido");
            } else {
                enviar_comando_uint32(socket_kernel, OP_CONSOLA_KILL, (uint32_t)pid);
                log_info(logger_consola, "Enviado KILL PID=%d", pid);
            }
            break;
        }

        case CMD_PS: {
            enviar_comando_simple(socket_kernel, OP_CONSOLA_PS);
            log_info(logger_consola, "Enviado PS, esperando respuesta...");

            // Esperar respuesta del kernel
            t_paquete* resp = recibir_paquete(socket_kernel);
            if (resp && resp->codigo_operacion == OP_CONSOLA_PS_RESP) {
                char* texto = paquete_read_string(resp);
                if (texto) {
                    printf("\n%s\n", texto);
                    free(texto);
                }
            } else {
                log_error(logger_consola, "No se recibio respuesta de PS");
            }
            if (resp) paquete_destroy(resp);
            break;
        }

        case CMD_ALGORITMO:
            if (!tokens[1]) {
                log_error(logger_consola, "ALGORITMO requiere FIFO | RR | VRR | HRRN");
            } else {
                enviar_comando_string(socket_kernel, OP_CONSOLA_ALGORITMO, tokens[1]);
                log_info(logger_consola, "Enviado ALGORITMO: %s", tokens[1]);
            }
            break;

        case CMD_START:
            enviar_comando_simple(socket_kernel, OP_CONSOLA_START);
            log_info(logger_consola, "Enviado START");
            break;

        case CMD_PAUSE:
            enviar_comando_simple(socket_kernel, OP_CONSOLA_PAUSE);
            log_info(logger_consola, "Enviado PAUSE");
            break;

        case CMD_DESALOJAR: {
            int pid = obtener_pid(tokens[1]);
            if (pid == -1) {
                log_error(logger_consola, "DESALOJAR requiere un PID valido");
            } else {
                enviar_comando_uint32(socket_kernel, OP_CONSOLA_DESALOJAR, (uint32_t)pid);
                log_info(logger_consola, "Enviado DESALOJAR PID=%d", pid);
            }
            break;
        }

        case CMD_EXIT:
            log_info(logger_consola, "Cerrando consola...");
            enviar_comando_simple(socket_kernel, OP_CONSOLA_EXIT);
            string_iterate_lines(tokens, (void(*)(char*)) free);
            free(tokens);
            return true;

        case CMD_HELP:
            mensaje_inicial();
            break;

        case CMD_UNKNOWN:
        default:
            log_error(logger_consola, "Comando no reconocido. Escriba HELP para ver los comandos disponibles.");
            break;
    }

    string_iterate_lines(tokens, (void(*)(char*)) free);
    free(tokens);
    return false;
}

// ============================
// API publica
// ============================

void consola_init(const char* config_path)
{
    // Crear logger
    logger_consola = log_create("consola.log", "CONSOLA", 1, LOG_LEVEL_INFO);
    if (!logger_consola) {
        perror("No se pudo crear logger de consola");
        exit(EXIT_FAILURE);
    }

    // Cargar config
    config_consola = consola_cargar_config(config_path);
    log_info(logger_consola, "Config cargada: IP_KERNEL=%s, PUERTO_KERNEL=%s",
             config_consola.ip_kernel, config_consola.puerto_kernel);

    // Conectar al kernel
    socket_kernel = crear_conexion(config_consola.ip_kernel, config_consola.puerto_kernel);
    if (socket_kernel < 0) {
        log_error(logger_consola, "No se pudo conectar al Kernel en %s:%s",
                  config_consola.ip_kernel, config_consola.puerto_kernel);
        exit(EXIT_FAILURE);
    }
    log_info(logger_consola, "Conectado al Kernel (FD=%d)", socket_kernel);

    // Handshake
    bool ok = handshake_cliente(socket_kernel, OP_HANDSHAKE, OP_OK, logger_consola);
    if (!ok) {
        log_error(logger_consola, "Handshake con Kernel fallido");
        close(socket_kernel);
        exit(EXIT_FAILURE);
    }
    log_info(logger_consola, "Handshake con Kernel exitoso");

    // Inicializar comandos
    init_comandos();
}

void consola_run(void)
{
    mensaje_inicial();

    char* linea;

    while ((linea = readline("consola> ")) != NULL) {

        if (string_is_empty(linea)) {
            free(linea);
            continue;
        }

        add_history(linea);

        bool salir = procesar_linea(linea);
        free(linea);

        if (salir)
            break;
    }
}

void consola_shutdown(void)
{
    log_info(logger_consola, "Apagando consola...");

    destroy_comandos();

    if (socket_kernel >= 0) {
        close(socket_kernel);
        socket_kernel = -1;
    }

    if (config_consola.ip_kernel) {
        free(config_consola.ip_kernel);
        config_consola.ip_kernel = NULL;
    }
    if (config_consola.puerto_kernel) {
        free(config_consola.puerto_kernel);
        config_consola.puerto_kernel = NULL;
    }

    if (logger_consola) {
        log_destroy(logger_consola);
        logger_consola = NULL;
    }
}
