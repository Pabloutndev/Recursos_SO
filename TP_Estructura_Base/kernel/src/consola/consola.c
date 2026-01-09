#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <consola/consola.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <loggers/logger.h>

#include <peticiones/proceso.h>
#include <peticiones/interrupciones.h>
#include <peticiones/dispatch.h>
#include <planificacion/planificacion.h>
#include <mod_kernel.h>

#include <commons/collections/dictionary.h>
#include <commons/string.h>

static t_dictionary* comandos_dict;

void init_comandos(void)
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
}

bool procesar_linea(char* linea)
{
    char** tokens = string_split(linea, " ");

    if (!tokens[0]) {
        void* f = free;
        string_iterate_lines(tokens, f);
        free(tokens);
        return false;
    }

    comando_t cmd = obtener_comando(tokens[0]);

    switch (cmd) {

        case CMD_RUN:
            if (tokens[1])
                ejecutar_proceso(tokens[1]);
            else
                log_error(logger, "RUN requiere un path");
            break;

        case CMD_KILL: {
            int pid = obtener_pid(tokens[1]);
            if (pid == -1)
                log_error(logger, "KILL requiere un PID válido");
            else
                matar_proceso(pid);
            break;
        }

        case CMD_PS:
            mostrar_procesos();
            break;

        case CMD_ALGORITMO:
            if (!tokens[1]) {
                log_error(logger, "ALGORITMO requiere FIFO | RR | HRRN");
                break;
            }

            if (strcmp(tokens[1], "FIFO") == 0)
                set_algoritmo(ALG_FIFO);
            else if (strcmp(tokens[1], "RR") == 0)
                set_algoritmo(ALG_RR);
            else if (strcmp(tokens[1], "HRRN") == 0)
                set_algoritmo(ALG_HRRN);
            else {
                log_error(logger, "Algoritmo no válido");
                break;
            }

            log_info(logger, "Algoritmo cambiado correctamente");
            break;

        case CMD_START:
            planificacion_start();
            break;

        case CMD_PAUSE:
            planificacion_pause();
            break;

        case CMD_DESALOJAR: {
            int pid = obtener_pid(tokens[1]);
            if (pid == -1)
                log_error(logger, "DESALOJAR requiere un PID válido");
            else
                desalojar_proceso(pid);
            break;
        }

        case CMD_EXIT:
            log_info(logger, "Cerrando consola...");
            kernel_shutdown();
            void* f = free;
            string_iterate_lines(tokens, f);
            free(tokens);
            return true;

        case CMD_UNKNOWN:
        default:
            log_error(logger, "Comando no reconocido");
            break;
    }
    void* f = free;
    string_iterate_lines(tokens, f);
    free(tokens);
    return false;
}

void iniciar_consola(void)
{
    mensaje_inicial();
    init_comandos();

    char* linea;

    while ((linea = readline("kernel> ")) != NULL) {

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

    dictionary_destroy(comandos_dict);
}

/* =========================
 *  Utilidades
 * ========================= */

void mensaje_inicial(void)
{
    printf("=== Consola del Kernel ===\n");
    printf("Comandos disponibles:\n");
    printf("  RUN <path>               - Crear y ejecutar proceso\n");
    printf("  KILL <pid>               - Terminar proceso\n");
    printf("  PS                       - Listar procesos por estado\n");
    printf("  ALGORITMO <FIFO|RR|HRRN> - Cambiar algoritmo de planificación\n");
    printf("  START                    - Iniciar planificación\n");
    printf("  PAUSE                    - Pausar planificación\n");
    printf("  DESALOJAR <pid>          - Desalojar proceso en ejecución\n");
    printf("  EXIT                     - Salir\n");
    printf("========================\n");
}

comando_t obtener_comando(const char* palabra)
{
    char* cmd = string_duplicate((char*)palabra);
    string_to_upper(cmd);

    comando_t resultado = CMD_UNKNOWN;

    if (dictionary_has_key(comandos_dict, cmd))
        resultado = (comando_t) dictionary_get(comandos_dict, cmd);

    free(cmd);
    return resultado;
}

int obtener_pid(char* token)
{
    if (!token)
        return -1;

    int pid = atoi(token);
    return pid > 0 ? pid : -1;
}

void comandos_destroy(void) {
    dictionary_destroy(comandos_dict);
}