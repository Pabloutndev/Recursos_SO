#include <stdlib.h>
#include <consola/consola.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <utils/logger.h>
#include <peticiones/proceso.h>
#include <peticiones/interrupciones.h>

void iniciar_consola(void)
{
    char* linea;

    while((linea = readline("> ")) != NULL) {
        add_history(linea);

        if (strncmp(linea, "RUN", 3) == 0) {
            ejercutar_proceso(linea + 4);
        } 
        else if (strncmp(linea, "KILL", 4) == 0) {
            matar_proceso(atoi(linea + 5));
        }
        else if (strcmp(linea, "PS") == 0) {
            mostrar_procesos();
        }
        else if (strcmp(linea, "EXIT") == 0) {
            free(linea);
            break;
        }

        free(linea);
    }
}