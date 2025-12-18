// planificacion.c
#include <pcb/pcb.h>
#include <planificacion/planificacion.h>
#include <planificacion/algoritmo.h>
#include <planificacion/corto_plazo.h>
#include <planificacion/largo_plazo.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <string.h>
#include <config/kernel_config.h>
#include <loggers/logger.h>

extern t_log* logger;
extern t_kernel_config KCONF;

/* Colas */
t_list* cola_new;
t_list* cola_ready;
t_list* cola_exec;   // existe pero la gestion de exec la hace dispatch
t_list* cola_blocked;
t_list* cola_exit;

/* Mutex */
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_blocked;

/* Semaforos */
sem_t sem_hay_new;
sem_t sem_hay_ready;
sem_t sem_mp;

/* Algoritmo */
static algoritmo_t algoritmo_actual = ALG_FIFO;
t_pcb* (*proximoAEjecutar)(void) = NULL;

/* Prototipos (algoritmos implementados en algoritmo.c) */
extern t_pcb* algoritmo_obtener_fifo(void);
extern t_pcb* algoritmo_obtener_rr(void);
extern t_pcb* algoritmo_obtener_hrrn(void);

/* Hilos */
static pthread_t hilo_largo, hilo_corto;

/* Inicialización */
void planificacion_init(void)
{
    cola_new    = list_create();
    cola_ready  = list_create();
    cola_exec   = list_create();
    cola_blocked= list_create();
    cola_exit   = list_create();

    pthread_mutex_init(&mutex_new, NULL);
    pthread_mutex_init(&mutex_ready, NULL);
    pthread_mutex_init(&mutex_exec, NULL);
    pthread_mutex_init(&mutex_blocked, NULL);

    sem_init(&sem_hay_new, 0, 0);
    sem_init(&sem_hay_ready, 0, 0);
    sem_init(&sem_mp, 0, KCONF.grado_multiprogramacion);

    /* seleccionar algoritmo desde config */
    if (strcmp(KCONF.algoritmo_planificacion, "RR") == 0) algoritmo_actual = ALG_RR;
    else if (strcmp(KCONF.algoritmo_planificacion, "HRRN") == 0) algoritmo_actual = ALG_HRRN;
    else algoritmo_actual = ALG_FIFO;

    set_algoritmo(algoritmo_actual);

    pthread_create(&hilo_largo, NULL, planificador_largo_plazo, NULL);
    pthread_create(&hilo_corto, NULL, planificador_corto_plazo, NULL);
}

/* Destrucción */
void planificacion_destroy(void)
{
    sem_post(&sem_hay_new);   // despertar hilos si están bloqueados
    sem_post(&sem_hay_ready);

    pthread_join(hilo_largo, NULL);
    pthread_join(hilo_corto, NULL);

    list_destroy_and_destroy_elements(cola_new,    (void(*)(void*)) pcb_destruir);
    list_destroy_and_destroy_elements(cola_ready,  (void(*)(void*)) pcb_destruir);
    list_destroy_and_destroy_elements(cola_exec,   (void(*)(void*)) pcb_destruir);
    list_destroy_and_destroy_elements(cola_blocked,(void(*)(void*)) pcb_destruir);
    list_destroy_and_destroy_elements(cola_exit,   (void(*)(void*)) pcb_destruir);

    pthread_mutex_destroy(&mutex_new);
    pthread_mutex_destroy(&mutex_ready);
    pthread_mutex_destroy(&mutex_exec);
    pthread_mutex_destroy(&mutex_blocked);

    sem_destroy(&sem_hay_new);
    sem_destroy(&sem_hay_ready);
    sem_destroy(&sem_mp);
}

/* Ingresar a NEW */
void ingresar_new(t_pcb* pcb)
{
    pthread_mutex_lock(&mutex_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_new);
    sem_post(&sem_hay_new);
}

/* Set algoritmo: apunta proximoAEjecutar a la función adecuada */
void set_algoritmo(algoritmo_t a)
{
    algoritmo_actual = a;
    switch (a) {
        case ALG_FIFO: proximoAEjecutar = algoritmo_obtener_fifo; break;
        case ALG_RR:   proximoAEjecutar = algoritmo_obtener_rr;   break;
        case ALG_HRRN: proximoAEjecutar = algoritmo_obtener_hrrn; break;
        default:       proximoAEjecutar = algoritmo_obtener_fifo; break;
    }
    log_info(logger, "Algoritmo seteado: %d", algoritmo_actual);
}

void listar_procesos_por_estado(void)
{
    log_info(logger, "Estado: NEW - Procesos: %s", lista_pids(cola_new));
    log_info(logger, "Estado: READY - Procesos: %s", lista_pids(cola_ready));
    log_info(logger, "Estado: EXEC - Procesos: %s", lista_pids(cola_exec));
    log_info(logger, "Estado: BLOCKED - Procesos: %s", lista_pids(cola_blocked));
    log_info(logger, "Estado: EXIT - Procesos: %s", lista_pids(cola_exit));
}
