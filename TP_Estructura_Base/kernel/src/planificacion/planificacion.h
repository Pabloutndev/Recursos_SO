#ifndef PLANIFICACION_H_
#define PLANIFICACION_H

#include <pcb/pcb.h>
#include <pthread.h>

typedef enum {
    ALG_FIFO,
    ALG_RR,
    ALG_HRRN,
    ALG_SJF,
    ALG_SRT,
    ALG_PRIORIDAD
} algoritmo_t;

typedef enum {
    PLANIF_STOPPED,
    PLANIF_RUNNING,
    PLANIF_PAUSED
} planif_state_t;

void planificacion_init(void);
void planificacion_start(void);
void planificacion_pause(void);
void planificacion_destroy(void);

void planificacion_ingresar_proceso(t_pcb* pcb);
void listar_procesos_por_estado(void);
void planificacion_matar_proceso(t_pcb* pcb);
void planificacion_dump_estado(t_pcb* pcb);

void ingresar_new(t_pcb* pcb);
void set_algoritmo(algoritmo_t a);

#endif /*PLANIFICACION_H*/