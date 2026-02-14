// consola.c - Modulo Consola Standalone
#include <consola.h>
#include <config/consola_config.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>

#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>

// Tipos de comando (local al modulo consola)
typedef enum {
    CMD_RUN,
    CMD_KILL,
    CMD_PS,
    CMD_ALGORITMO,
    CMD_START,
    CMD_PAUSE,
    CMD_DESALOJAR,
    CMD_EXIT,
    CMD_HELP,
    CMD_UNKNOWN
} comando_t;

// Estado global de la consola
static t_log* logger_consola = NULL;
static t_consola_config config_consola;
static int socket_kernel = -1;
static t_dictionary* comandos_dict = NULL;

// ============================
// Helpers locales
// ============================

static void init_comandos(void)
{
    comandos_dict = dictionary_create();

    dictionary_put(comandos_dict, "RUN",       (void*) CMD_RUN);
    dictionary_put(comandos_dict, "KILL",      (void*) CMD_KILL);
    dictionary_put(comandos_dict, "PS",        (void*) CMD_PS);
    dictionary_put(comandos_dict, "ALGORITMO", (void*) CMD_ALGORITMO);
    dictionary_put(comandos_dict, "START",     (void*) CMD_START);
    dictionary_put(comandos_dict, "PAUSE",     (void*) CMD_PAUSE);
    dictionary_put(comandos_dict, "DESALOJAR", (void*) CMD_DESALOJAR);
    dictionary_put(comandos_dict, "EXIT",      (void*) CMD_EXIT);
    dictionary_put(comandos_dict, "HELP",      (void*) CMD_HELP);
}

static comando_t obtener_comando(const char* palabra)
{
    char* cmd = string_duplicate((char*)palabra);
    string_to_upper(cmd);

    comando_t resultado = CMD_UNKNOWN;

    if (dictionary_has_key(comandos_dict, cmd))
        resultado = (comando_t)(intptr_t) dictionary_get(comandos_dict, cmd);

    free(cmd);
    return resultado;
}

static int obtener_pid(char* token)
{
    if (!token)
        return -1;

    int pid = atoi(token);
    return pid > 0 ? pid : -1;
}

static void mensaje_inicial(void)
{
    printf("=== Consola Remota del Kernel ===\n");
    printf("Comandos disponibles:\n");
    printf("  RUN <nombre>             - Crear y ejecutar proceso\n");
    printf("  KILL <pid>               - Terminar proceso\n");
    printf("  PS                       - Listar procesos por estado\n");
    printf("  ALGORITMO <FIFO|RR|VRR|HRRN> - Cambiar algoritmo de planificacion\n");
    printf("  START                    - Iniciar planificacion\n");
    printf("  PAUSE                    - Pausar planificacion\n");
    printf("  DESALOJAR <pid>          - Desalojar proceso en ejecucion\n");
    printf("  HELP                     - Mostrar esta ayuda\n");
    printf("  EXIT                     - Salir\n");
    printf("=================================\n");
}

// ============================
// Envio de comandos al Kernel
// ============================

static void enviar_comando_string(op_code code, const char* str)
{
    t_paquete* p = paquete_create(code);
    paquete_write_string(p, str);
    enviar_paquete(socket_kernel, p);
    paquete_destroy(p);
}

static void enviar_comando_uint32(op_code code, uint32_t val)
{
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, val);
    enviar_paquete(socket_kernel, p);
    paquete_destroy(p);
}

static void enviar_comando_simple(op_code code)
{
    t_paquete* p = paquete_create(code);
    enviar_paquete(socket_kernel, p);
    paquete_destroy(p);
}

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
                enviar_comando_string(OP_CONSOLA_RUN, tokens[1]);
                log_info(logger_consola, "Enviado RUN: %s", tokens[1]);
            } else {
                log_error(logger_consola, "RUN requiere un path");
            }
            break;

        case CMD_KILL: {
            int pid = obtener_pid(tokens[1]);
            if (pid == -1) {
                log_error(logger_consola, "KILL requiere un PID valido");
            } else {
                enviar_comando_uint32(OP_CONSOLA_KILL, (uint32_t)pid);
                log_info(logger_consola, "Enviado KILL PID=%d", pid);
            }
            break;
        }

        case CMD_PS: {
            enviar_comando_simple(OP_CONSOLA_PS);
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
                enviar_comando_string(OP_CONSOLA_ALGORITMO, tokens[1]);
                log_info(logger_consola, "Enviado ALGORITMO: %s", tokens[1]);
            }
            break;

        case CMD_START:
            enviar_comando_simple(OP_CONSOLA_START);
            log_info(logger_consola, "Enviado START");
            break;

        case CMD_PAUSE:
            enviar_comando_simple(OP_CONSOLA_PAUSE);
            log_info(logger_consola, "Enviado PAUSE");
            break;

        case CMD_DESALOJAR: {
            int pid = obtener_pid(tokens[1]);
            if (pid == -1) {
                log_error(logger_consola, "DESALOJAR requiere un PID valido");
            } else {
                enviar_comando_uint32(OP_CONSOLA_DESALOJAR, (uint32_t)pid);
                log_info(logger_consola, "Enviado DESALOJAR PID=%d", pid);
            }
            break;
        }

        case CMD_EXIT:
            log_info(logger_consola, "Cerrando consola...");
            enviar_comando_simple(OP_CONSOLA_EXIT);
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

    if (comandos_dict) {
        dictionary_destroy(comandos_dict);
        comandos_dict = NULL;
    }

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
