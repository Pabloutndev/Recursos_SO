#include <stdlib.h>
#include <string.h>
#include <consola/consola.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <loggers/logger.h>
#include <peticiones/proceso.h>
#include <peticiones/interrupciones.h>
#include <peticiones/dispatch.h>
#include <planificacion/planificacion.h>
#include <mod_kernel.h>

extern t_log* logger;

void iniciar_consola(void)
{
    mensaje_inicial();
    
    char* linea;

    while((linea = readline("kernel> ")) != NULL) {
        if (strlen(linea) == 0) {
            free(linea);
            continue;
        }
        
        add_history(linea);

        if (strncmp(linea, "RUN", 3) == 0) {
            char* path = linea + 4;
            while (*path == ' ') path++; // Saltar espacios
            if (*path != '\0') {
                ejecutar_proceso(path);
            } else {
                log_error(logger, "RUN requiere un path");
            }
        } 
        else if (strncmp(linea, "KILL", 4) == 0) {
            int pid = atoi(linea + 5);
            if (pid > 0) {
                matar_proceso(pid);
            } else {
                log_error(logger, "KILL requiere un PID válido");
            }
        }
        else if (strcmp(linea, "PS") == 0) {
            mostrar_procesos();
        }
        else if (strncmp(linea, "ALGORITMO", 9) == 0) {
            char* alg_str = linea + 10;
            while (*alg_str == ' ') alg_str++;
            
            algoritmo_t alg;
            if (strcmp(alg_str, "FIFO") == 0) {
                alg = ALG_FIFO;
            } else if (strcmp(alg_str, "RR") == 0) {
                alg = ALG_RR;
            } else if (strcmp(alg_str, "HRRN") == 0) {
                alg = ALG_HRRN;
            } else {
                log_error(logger, "Algoritmo no válido. Use: FIFO, RR, HRRN");
                free(linea);
                continue;
            }
            set_algoritmo(alg);
            log_info(logger, "Algoritmo cambiado correctamente");
        }
        else if (strcmp(linea, "START") == 0) {
            planificacion_start();
        }
        else if (strcmp(linea, "PAUSE") == 0) {
            planificacion_pause();
        }
        else if (strncmp(linea, "DESALOJAR", 9) == 0) {
            int pid = atoi(linea + 10);
            if (pid > 0) {
                desalojar_proceso(pid);
            } else {
                log_error(logger, "DESALOJAR requiere un PID válido");
            }
        }
        else if (strcmp(linea, "EXIT") == 0) {
            free(linea);
            log_info(logger, "Cerrando consola...");
            kernel_shutdown();
            break;
        }
        else {
            log_error(logger, "Comando no reconocido: %s", linea);
        }

        free(linea);
    }
}

void mensaje_inicial(void)
{
    printf("=== Consola del Kernel ===\n");
    printf("Comandos disponibles:\n");
    printf("  RUN <path>          - Crear y ejecutar proceso\n");
    printf("  KILL <pid>          - Terminar proceso\n");
    printf("  PS                  - Listar procesos por estado\n");
    printf("  ALGORITMO <FIFO|RR|HRRN> - Cambiar algoritmo de planificación\n");
    printf("  START               - Iniciar planificación\n");
    printf("  PAUSE               - Pausar planificación\n");
    printf("  DESALOJAR <pid>     - Desalojar proceso en ejecución\n");
    printf("  EXIT                - Salir\n");
    printf("========================\n");
}