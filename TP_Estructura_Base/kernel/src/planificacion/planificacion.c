// planificacion.c
#include <pcb/pcb.h>
#include <planificacion/planificacion.h>
#include <planificacion/algoritmo.h>
#include <planificacion/corto_plazo.h>
#include <planificacion/largo_plazo.h>
#include <mod_kernel.h>
#include <conexiones/memoria.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <string.h>
#include <config/kernel_config.h>
#include <loggers/logger.h>
#include <peticiones/dispatch.h>
#include <peticiones/recursos.h>

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
pthread_mutex_t mutex_exit = PTHREAD_MUTEX_INITIALIZER;

/* Semaforos */
sem_t sem_hay_new;
sem_t sem_hay_ready;
sem_t sem_mp;


/* Prototipos (algoritmos implementados en algoritmo.c) */
extern t_pcb* algoritmo_obtener_fifo(void);
extern t_pcb* algoritmo_obtener_rr(void);
extern t_pcb* algoritmo_obtener_hrrn(void);
extern t_pcb* algoritmo_obtener_vrr(void);

/* Hilos */
static pthread_t hilo_largo, hilo_corto;

/* Estado de planificación */
static planif_state_t estado_planificacion = PLANIF_STOPPED;
pthread_mutex_t mutex_estado_planif;
pthread_cond_t cond_planif_resume;

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
    pthread_cond_init(&cond_planif_resume, NULL);

    sem_init(&sem_hay_new, 0, 0);
    sem_init(&sem_hay_ready, 0, 0);
    sem_init(&sem_mp, 0, KERNEL_CTX.config.grado_multiprogramacion);


    /* seleccionar algoritmo desde config */
    if (strcmp(KERNEL_CTX.config.algoritmo_planificacion, "RR") == 0) algoritmo_actual = ALG_RR;
    else if (strcmp(KERNEL_CTX.config.algoritmo_planificacion, "VRR") == 0) algoritmo_actual = ALG_VRR;
    else if (strcmp(KERNEL_CTX.config.algoritmo_planificacion, "HRRN") == 0) algoritmo_actual = ALG_HRRN;
    else algoritmo_actual = ALG_FIFO;

    set_algoritmo(algoritmo_actual);

    estado_planificacion = PLANIF_STOPPED;
}

/* Destrucción */
void planificacion_destroy(void)
{
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
    pthread_mutex_destroy(&mutex_exit);
    pthread_mutex_destroy(&mutex_estado_planif);
    pthread_cond_destroy(&cond_planif_resume);

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
    const char* nombre;
    switch (a) {
        case ALG_FIFO: proximoAEjecutar = algoritmo_obtener_fifo; nombre = "FIFO"; break;
        case ALG_RR:   proximoAEjecutar = algoritmo_obtener_rr;   nombre = "RR";   break;
        case ALG_VRR:  proximoAEjecutar = algoritmo_obtener_vrr;  nombre = "VRR";  break;
        case ALG_HRRN: proximoAEjecutar = algoritmo_obtener_hrrn; nombre = "HRRN"; break;
        default:       proximoAEjecutar = algoritmo_obtener_fifo; nombre = "FIFO"; break;
    }
    // Actualizar string de config para que timer_quantum y otros lo vean
    free(KERNEL_CTX.config.algoritmo_planificacion);
    KERNEL_CTX.config.algoritmo_planificacion = strdup(nombre);
    log_info(KERNEL_CTX.logger, "Algoritmo seteado: %s", KERNEL_CTX.config.algoritmo_planificacion);
}

void listar_procesos_por_estado(void)
{
    char* pids_new = lista_pids(cola_new);
    char* pids_ready = lista_pids(cola_ready);
    char* pids_exec = lista_pids(cola_exec);
    char* pids_blocked = lista_pids(cola_blocked);
    char* pids_exit = lista_pids(cola_exit);
    
    log_info(KERNEL_CTX.logger, "Estado: NEW - Procesos: [%s]", pids_new ? pids_new : "");
    log_info(KERNEL_CTX.logger, "Estado: READY - Procesos: [%s]", pids_ready ? pids_ready : "");
    log_info(KERNEL_CTX.logger, "Estado: EXEC - Procesos: [%s]", pids_exec ? pids_exec : "");
    log_info(KERNEL_CTX.logger, "Estado: BLOCKED - Procesos: [%s]", pids_blocked ? pids_blocked : "");
    log_info(KERNEL_CTX.logger, "Estado: EXIT - Procesos: [%s]", pids_exit ? pids_exit : "");
    
    if (pids_new) free(pids_new);
    if (pids_ready) free(pids_ready);
    if (pids_exec) free(pids_exec);
    if (pids_blocked) free(pids_blocked);
    if (pids_exit) free(pids_exit);
}

/* Chequear si la planificacion esta pausada; bloquea hasta que se reanude */
void planificacion_check_pause(void)
{
    pthread_mutex_lock(&mutex_estado_planif);
    while (estado_planificacion == PLANIF_PAUSED) {
        log_info(KERNEL_CTX.logger, "Planificacion pausada - esperando START...");
        pthread_cond_wait(&cond_planif_resume, &mutex_estado_planif);
    }
    pthread_mutex_unlock(&mutex_estado_planif);
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
    } else if (estado_planificacion == PLANIF_PAUSED) {
        estado_planificacion = PLANIF_RUNNING;
        pthread_cond_broadcast(&cond_planif_resume);
        log_info(KERNEL_CTX.logger, "Planificacion reanudada");
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
void planificacion_finalizar_proceso(uint32_t pid)
{
    pcb_search_pid = pid; // Usamos el static para list_find
    t_pcb* encontrado = NULL;
    bool estaba_en_new = false;

    // 1. Intentar NEW
    pthread_mutex_lock(&mutex_new);
    encontrado = list_find(cola_new, pcb_equals_pid);
    if (encontrado) {
        list_remove_element(cola_new, encontrado);
        estaba_en_new = true;
    }
    pthread_mutex_unlock(&mutex_new);

    // 2. Intentar READY
    if (!encontrado) {
        pthread_mutex_lock(&mutex_ready);
        encontrado = list_find(cola_ready, pcb_equals_pid);
        if (encontrado) list_remove_element(cola_ready, encontrado);
        pthread_mutex_unlock(&mutex_ready);
    }

    // 3. Intentar BLOCKED
    if (!encontrado) {
        pthread_mutex_lock(&mutex_blocked);
        encontrado = list_find(cola_blocked, pcb_equals_pid);
        if (encontrado) list_remove_element(cola_blocked, encontrado);
        pthread_mutex_unlock(&mutex_blocked);
    }

    // 4. Intentar EXEC
    if (!encontrado) {
        pthread_mutex_lock(&mutex_exec);
        encontrado = list_find(cola_exec, pcb_equals_pid);
        if (encontrado) {
            list_remove_element(cola_exec, encontrado);
            enviar_interrupt_cpu(encontrado->pid);
        }
        pthread_mutex_unlock(&mutex_exec);
    }

    if (encontrado) {
        encontrado->estado = EXIT;
        pthread_mutex_lock(&mutex_exit);
        list_add(cola_exit, encontrado);
        pthread_mutex_unlock(&mutex_exit);

        log_fin_proceso(pid, "KILL/EXIT");

        if (!estaba_en_new) {
            // Solo notificar a memoria y liberar slot si el proceso fue inicializado
            solicitar_fin_proceso_memoria(pid);
            recursos_liberar_proceso(pid);
            sem_post(&sem_mp);
        }
    } else {
        log_error(KERNEL_CTX.logger, "Finalizar Proceso: PID %d no encontrado en ninguna cola", pid);
    }
    
    pcb_search_pid = 0;
} 
// Wrappers legacy
void planificacion_matar_proceso(t_pcb* pcb) {
    if(pcb) planificacion_finalizar_proceso(pcb->pid);
}

/* Dump estado de proceso */
void planificacion_dump_estado(t_pcb* pcb)
{
    if (!pcb) return;
    
    log_info(KERNEL_CTX.logger, "=== DUMP PROCESO PID: %u ===", pcb->pid);
    log_info(KERNEL_CTX.logger, "Estado: %d", pcb->estado);
    log_info(KERNEL_CTX.logger, "PC: %u", pcb->program_counter);
    log_info(KERNEL_CTX.logger, "Quantum: %d", pcb->quantum);
    log_info(KERNEL_CTX.logger, "Prioridad: %d", pcb->prioridad);
    log_info(KERNEL_CTX.logger, "Estimación ráfaga: %.2f", pcb->estimacion_rafaga);
    log_info(KERNEL_CTX.logger, "Tamaño proceso: %u", pcb->tam_proceso);
    log_info(KERNEL_CTX.logger, "===========================");
}
