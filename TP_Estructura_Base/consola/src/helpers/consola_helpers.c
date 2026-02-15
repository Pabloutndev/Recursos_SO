#include <helpers/consola_helpers.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>

static t_dictionary* comandos_dict = NULL;

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
    dictionary_put(comandos_dict, "HELP",      (void*) CMD_HELP);
}

void destroy_comandos(void)
{
    if (comandos_dict) {
        dictionary_destroy(comandos_dict);
        comandos_dict = NULL;
    }
}

comando_t obtener_comando(const char* palabra)
{
    char* cmd = string_duplicate((char*)palabra);
    string_to_upper(cmd);

    comando_t resultado = CMD_UNKNOWN;

    if (dictionary_has_key(comandos_dict, cmd))
        resultado = (comando_t)(intptr_t) dictionary_get(comandos_dict, cmd);

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

void mensaje_inicial(void)
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
