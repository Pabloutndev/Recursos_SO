#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

typedef struct
{   
    char* puerto_escucha;
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_cpu;
    char* puerto_cpu_dispatch;
    char* puerto_cpu_interrupt;
    char* algoritmo_planificacion;
    int quantum;
    char** recursos; //puntero a punteros de strings
    char** instancias_recursos;
    int grado_multiprogramacion;
} t_kernel_config;

t_kernel_config kernel_cargar_config(const char* ruta);
void kernel_imprimir_config(t_kernel_config kernel);

#endif /* KERNEL_CONFIG_H_ */