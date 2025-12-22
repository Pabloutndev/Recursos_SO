#ifndef CPU_CONFIG_H
#define CPU_CONFIG_H

typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_kernel;
    char* puerto_kernel;
    char* puerto_dispatch;
    char* puerto_interrupt;
    int   tlb_cant_ent;
    char* tlb_algoritmo;
} t_cpu_config;

t_cpu_config cpu_cargar_config(const char* ruta);
void cpu_imprimir_config(t_cpu_config cpu);

#endif
