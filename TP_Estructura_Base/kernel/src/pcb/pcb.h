#ifndef PCB_H
#define PCB_H

#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>

#include <model/model.h>

typedef enum {
    NEW, 
    READY,
    EXEC,
    BLOCK,
    EXIT
} t_estado;

typedef struct {
    uint32_t pid;
    char* path;
    // t_list* instrucciones;
    uint32_t program_counter;
    t_estado estado;
    int quantum;
    int quantum_restante; // VRR: quantum que le queda al proceso
    
    registros_t registros;// registros generales del proceso
    
    // administracion de memoria
    t_list* tabla_segmentos; // tabla de segmentos (id, base, size) o paginas
    uint32_t tam_proceso;
    
    // archivos abiertos: lista de (nombre, offset) del proceso
    t_list* tabla_archivos;
    
    // Estadistica (EXTRAS AL KERNEL)
    int prioridad;              // (0 = mayor prioridad)
    double estimacion_rafaga;   // Algoritmo HRRN - SJF
    t_temporal* tiempo_ready;
    t_temporal* tiempo_inicio_exec;
    
    // skt = Opcional para notificacion
    int socket_consola;
} t_pcb;

typedef struct {
    uint32_t tid;
    int prioridad;
    // registro/local state del hilo si hace falta
} t_tcb;

t_pcb* pcb_crear(void);
uint32_t generar_pid(void);
void pcb_destruir(t_pcb* pcb);

#endif /* PCB_H */