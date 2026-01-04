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

/* Prototipos (algoritmos implementados en algoritmo.c) */
extern t_pcb* algoritmo_obtener_fifo(void);
extern t_pcb* algoritmo_obtener_rr(void);
extern t_pcb* algoritmo_obtener_hrrn(void);

/* Hilos */
static pthread_t hilo_largo, hilo_corto;

/* Estado de planificación */
static planif_state_t estado_planificacion = PLANIF_STOPPED;
pthread_mutex_t mutex_estado_planif;

/* Algoritmo */
static algoritmo_t algoritmo_actual = ALG_FIFO;
t_pcb* (*proximoAEjecutar)(void) = NULL;

/* Variable estática temporal para búsqueda de PCB por PID */
static uint32_t pcb_search_pid = 0;

/* Función auxiliar para buscar PCB por PID (usada con list_find) */
static bool pcb_equals_pid(void* elem) {
    t_pcb* pcb = (t_pcb*) elem;
    return pcb && pcb->pid == pcb_search_pid;
}

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
    pthread_mutex_init(&mutex_estado_planif, NULL);

    sem_init(&sem_hay_new, 0, 0);
    sem_init(&sem_hay_ready, 0, 0);
    sem_init(&sem_mp, 0, KCONF.grado_multiprogramacion);

    /* seleccionar algoritmo desde config */
    if (strcmp(KCONF.algoritmo_planificacion, "RR") == 0) algoritmo_actual = ALG_RR;
    else if (strcmp(KCONF.algoritmo_planificacion, "HRRN") == 0) algoritmo_actual = ALG_HRRN;
    else algoritmo_actual = ALG_FIFO;

    set_algoritmo(algoritmo_actual);

    estado_planificacion = PLANIF_STOPPED;
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
    pthread_mutex_destroy(&mutex_estado_planif);

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
    char* pids_new = lista_pids(cola_new);
    char* pids_ready = lista_pids(cola_ready);
    char* pids_exec = lista_pids(cola_exec);
    char* pids_blocked = lista_pids(cola_blocked);
    char* pids_exit = lista_pids(cola_exit);
    
    log_info(logger, "Estado: NEW - Procesos: [%s]", pids_new ? pids_new : "");
    log_info(logger, "Estado: READY - Procesos: [%s]", pids_ready ? pids_ready : "");
    log_info(logger, "Estado: EXEC - Procesos: [%s]", pids_exec ? pids_exec : "");
    log_info(logger, "Estado: BLOCKED - Procesos: [%s]", pids_blocked ? pids_blocked : "");
    log_info(logger, "Estado: EXIT - Procesos: [%s]", pids_exit ? pids_exit : "");
    
    if (pids_new) free(pids_new);
    if (pids_ready) free(pids_ready);
    if (pids_exec) free(pids_exec);
    if (pids_blocked) free(pids_blocked);
    if (pids_exit) free(pids_exit);
}

/* Iniciar planificación */
void planificacion_start(void)
{
    pthread_mutex_lock(&mutex_estado_planif);
    if (estado_planificacion == PLANIF_STOPPED) {
        estado_planificacion = PLANIF_RUNNING;
        pthread_create(&hilo_largo, NULL, planificador_largo_plazo, NULL);
        pthread_create(&hilo_corto, NULL, planificador_corto_plazo, NULL);
        log_inicio_planificacion();
    }
    pthread_mutex_unlock(&mutex_estado_planif);
}

/* Pausar planificación */
void planificacion_pause(void)
{
    pthread_mutex_lock(&mutex_estado_planif);
    if (estado_planificacion == PLANIF_RUNNING) {
        estado_planificacion = PLANIF_PAUSED;
        log_pausa_planificacion();
    }
    pthread_mutex_unlock(&mutex_estado_planif);
}

/* Matar proceso */
void planificacion_matar_proceso(t_pcb* pcb)
{
    if (!pcb) return;
    
    pcb_search_pid = pcb->pid;
    
    pthread_mutex_lock(&mutex_new);
    t_pcb* encontrado = list_find(cola_new, pcb_equals_pid);
    if (encontrado) {
        list_remove_element(cola_new, encontrado);
        encontrado->estado = EXIT;
        pthread_mutex_lock(&mutex_exec);
        list_add(cola_exit, encontrado);
        pthread_mutex_unlock(&mutex_exec);
        log_fin_proceso(encontrado->pid, "KILL");
        pthread_mutex_unlock(&mutex_new);
        pcb_search_pid = 0;
        return;
    }
    pthread_mutex_unlock(&mutex_new);
    
    pthread_mutex_lock(&mutex_ready);
    encontrado = list_find(cola_ready, pcb_equals_pid);
    if (encontrado) {
        list_remove_element(cola_ready, encontrado);
        encontrado->estado = EXIT;
        pthread_mutex_lock(&mutex_exec);
        list_add(cola_exit, encontrado);
        pthread_mutex_unlock(&mutex_exec);
        log_fin_proceso(encontrado->pid, "KILL");
        pthread_mutex_unlock(&mutex_ready);
        pcb_search_pid = 0;
        return;
    }
    pthread_mutex_unlock(&mutex_ready);
    
    pthread_mutex_lock(&mutex_exec);
    encontrado = list_find(cola_exec, pcb_equals_pid);
    if (encontrado) {
        list_remove_element(cola_exec, encontrado);
        encontrado->estado = EXIT;
        list_add(cola_exit, encontrado);
        log_fin_proceso(encontrado->pid, "KILL");
        // TODO: Enviar interrupción a CPU para detener ejecución
    }
    pthread_mutex_unlock(&mutex_exec);
    
    pthread_mutex_lock(&mutex_blocked);
    encontrado = list_find(cola_blocked, pcb_equals_pid);
    if (encontrado) {
        list_remove_element(cola_blocked, encontrado);
        encontrado->estado = EXIT;
        pthread_mutex_lock(&mutex_exec);
        list_add(cola_exit, encontrado);
        pthread_mutex_unlock(&mutex_exec);
        log_fin_proceso(encontrado->pid, "KILL");
    }
    pthread_mutex_unlock(&mutex_blocked);
    
    pcb_search_pid = 0;
}

/* Dump estado de proceso */
void planificacion_dump_estado(t_pcb* pcb)
{
    if (!pcb) return;
    
    log_info(logger, "=== DUMP PROCESO PID: %u ===", pcb->pid);
    log_info(logger, "Estado: %d", pcb->estado);
    log_info(logger, "PC: %u", pcb->program_counter);
    log_info(logger, "Quantum: %d", pcb->quantum);
    log_info(logger, "Prioridad: %d", pcb->prioridad);
    log_info(logger, "Estimación ráfaga: %.2f", pcb->estimacion_rafaga);
    log_info(logger, "Tamaño proceso: %u", pcb->tam_proceso);
    log_info(logger, "===========================");
}
