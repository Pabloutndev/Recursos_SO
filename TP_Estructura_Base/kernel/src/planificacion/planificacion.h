#ifndef PLANIFICACION_H
#define PLANIFICACION_H

#include <pcb/pcb.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/collections/list.h>

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

/* Colas */
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* cola_exec;
extern t_list* cola_blocked;
extern t_list* cola_exit;

/* Mutex */
extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_exec;
extern pthread_mutex_t mutex_blocked;
extern pthread_mutex_t mutex_estado_planif;

/* Semaforos */
extern sem_t sem_hay_new;
extern sem_t sem_hay_ready;
extern sem_t sem_mp;

/* Prototipos (algoritmos implementados en algoritmo.c) */
extern t_pcb* algoritmo_obtener_fifo(void);
extern t_pcb* algoritmo_obtener_rr(void);
extern t_pcb* algoritmo_obtener_hrrn(void);

/* Hilos */
static pthread_t hilo_largo;
static pthread_t hilo_corto;

/* Estado de planificacion */
extern algoritmo_t algoritmo_actual;
extern planif_state_t estado_planificacion;
extern t_pcb* (*proximoAEjecutar)(void);

/* api */
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