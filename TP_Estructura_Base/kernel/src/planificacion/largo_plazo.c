// largo_plazo.c
#include <planificacion/largo_plazo.h>
#include <pcb/pcb.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <semaphore.h>
#include <pthread.h>

extern sem_t sem_hay_new;
extern sem_t sem_mp;
extern sem_t sem_hay_ready;

extern pthread_mutex_t mutex_new;
extern pthread_mutex_t mutex_ready;

extern t_list* cola_new;
extern t_list* cola_ready;

extern t_log* logger;

void* planificador_largo_plazo(void* _) {
    while (1) {
        sem_wait(&sem_hay_new);
        sem_wait(&sem_mp);

        pthread_mutex_lock(&mutex_new);
        if (list_is_empty(cola_new)) {
            pthread_mutex_unlock(&mutex_new);
            sem_post(&sem_mp); // liberar slot ya que no hay proceso
            continue;
        }
        t_pcb* pcb = list_remove(cola_new, 0);
        pthread_mutex_unlock(&mutex_new);

        pcb->estado = READY;
        pcb->tiempo_ready = temporal_create();

        pthread_mutex_lock(&mutex_ready);
        list_add(cola_ready, pcb);
        pthread_mutex_unlock(&mutex_ready);

        log_info(logger, "NEW → READY PID=%u", pcb->pid);
        sem_post(&sem_hay_ready);
    }
    return NULL;
}
