// kernel_config.c
#include <stdio.h>
#include <stdlib.h>
#include <config/kernel_config.h>
#include <commons/config.h>

t_kernel_config kernel_cargar_config(const char* ruta)
{
    t_config* config_archivo = config_create((char*)ruta);
    t_kernel_config kernel;

    if(config_archivo==NULL) {
        perror("Archivo de configuracion del kernel no encontrado");
    }

    kernel.puerto_escucha            = config_get_string_value(config_archivo, "PUERTO_ESCUCHA");
    kernel.ip_memoria                = config_get_string_value(config_archivo, "IP_MEMORIA");
    kernel.puerto_memoria            = config_get_string_value(config_archivo, "PUERTO_MEMORIA");
    kernel.ip_cpu                    = config_get_string_value(config_archivo, "IP_CPU");
    kernel.puerto_cpu_dispatch       = config_get_string_value(config_archivo, "PUERTO_CPU_DISPATCH");
    kernel.puerto_cpu_interrupt      = config_get_string_value(config_archivo, "PUERTO_CPU_INTERRUPT");
    kernel.algoritmo_planificacion   = config_get_string_value(config_archivo, "ALGORITMO_PLANIFICACION");
    kernel.quantum                   = config_get_int_value(config_archivo, "QUANTUM");
    kernel.recursos                  = config_get_array_value(config_archivo,"RECURSOS");
    kernel.instancias_recursos       = config_get_array_value(config_archivo,"INSTANCIAS_RECURSOS");
    kernel.grado_multiprogramacion   = config_get_int_value(config_archivo, "GRADO_MULTIPROGRAMACION");

    // borrar config_archivo

    return kernel;
}

void kernel_imprimir_config(t_kernel_config kernel)
{
    printf("========== CONFIG KERNEL ==========\n");

    printf("PUERTO_ESCUCHA           : %s\n", kernel.puerto_escucha);
    printf("IP_MEMORIA               : %s\n", kernel.ip_memoria);
    printf("PUERTO_MEMORIA           : %s\n", kernel.puerto_memoria);
    printf("IP_CPU                   : %s\n", kernel.ip_cpu);
    printf("PUERTO_CPU_DISPATCH      : %s\n", kernel.puerto_cpu_dispatch);
    printf("PUERTO_CPU_INTERRUPT     : %s\n", kernel.puerto_cpu_interrupt);
    printf("ALGORITMO_PLANIFICACION  : %s\n", kernel.algoritmo_planificacion);
    printf("QUANTUM                  : %d\n", kernel.quantum);
    printf("GRADO_MULTIPROGRAMACION  : %d\n", kernel.grado_multiprogramacion);

    printf("RECURSOS                 : ");
    if (kernel.recursos != NULL) {
        for (int i = 0; kernel.recursos[i] != NULL; i++) {
            printf("%s ", kernel.recursos[i]);
        }
    }
    printf("\n");

    printf("INSTANCIAS_RECURSOS      : ");
    if (kernel.instancias_recursos != NULL) {
        for (int i = 0; kernel.instancias_recursos[i] != NULL; i++) {
            printf("%s ", kernel.instancias_recursos[i]);
        }
    }
    printf("\n");

    printf("===================================\n");
}