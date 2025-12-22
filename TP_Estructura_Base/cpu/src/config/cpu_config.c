// cpu_config.c
#include <stdio.h>
#include <stdlib.h>
#include <config/cpu_config.h>
#include <commons/config.h>

t_cpu_config cpu_cargar_config(const char* ruta)
{
    t_config* config_archivo = config_create((char*)ruta);
    t_cpu_config cpu;

    if (config_archivo == NULL) {
        perror("Archivo de configuracion del CPU no encontrado");
        exit(EXIT_FAILURE);
    }

    cpu.ip_memoria       = config_get_string_value(config_archivo, "IP_MEMORIA");
    cpu.puerto_memoria   = config_get_string_value(config_archivo, "PUERTO_MEMORIA");
    cpu.puerto_dispatch  = config_get_string_value(config_archivo, "PUERTO_ESCUCHA_DISPATCH");
    cpu.puerto_interrupt = config_get_string_value(config_archivo, "PUERTO_ESCUCHA_INTERRUPT");
    cpu.tlb_cant_ent     = config_get_int_value(config_archivo, "CANTIDAD_ENTRADAS_TLB");
    cpu.tlb_algoritmo    = config_get_string_value(config_archivo, "ALGORITMO_TLB");

    //config_destroy(config_archivo);

    return cpu;
}

void cpu_imprimir_config(t_cpu_config cpu)
{
    printf("========== CONFIG CPU ==========\n");

    printf("IP_MEMORIA               : %s\n", cpu.ip_memoria);
    printf("PUERTO_MEMORIA           : %s\n", cpu.puerto_memoria);
    printf("PUERTO_ESCUCHA_DISPATCH  : %s\n", cpu.puerto_dispatch);
    printf("PUERTO_ESCUCHA_INTERRUPT : %s\n", cpu.puerto_interrupt);
    printf("CANTIDAD_ENTRADAS_TLB    : %d\n", cpu.tlb_cant_ent);
    printf("ALGORITMO_TLB            : %s\n", cpu.tlb_algoritmo);

    printf("================================\n");
}
