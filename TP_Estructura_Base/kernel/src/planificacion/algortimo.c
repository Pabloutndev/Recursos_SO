// algoritmo.c
#include <planificacion/algoritmo.h>
#include <commons/temporal.h>
#include <pthread.h>
#include <commons/log.h>

extern t_list* cola_ready;
extern pthread_mutex_t mutex_ready;
extern t_log* logger;

/* HRRN - calculo auxiliar */
static double calcular_respuesta(t_pcb* pcb)
{
    if (!pcb->tiempo_ready) return 1.0;

    temporal_stop(pcb->tiempo_ready);
    double wait = temporal_gettime(pcb->tiempo_ready);
    temporal_resume(pcb->tiempo_ready);

    double est = pcb->estimacion_rafaga > 0 ? pcb->estimacion_rafaga : 1.0;
    return (wait + est) / est;
}

t_pcb* algoritmo_obtener_fifo(void)
{
    pthread_mutex_lock(&mutex_ready);
    if (list_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_ready);
        return NULL;
    }
    t_pcb* pcb = list_remove(cola_ready, 0);
    pthread_mutex_unlock(&mutex_ready);
    return pcb;
}

t_pcb* algoritmo_obtener_rr(void) {
    return algoritmo_obtener_fifo();
}

t_pcb* algoritmo_obtener_hrrn(void)
{
    pthread_mutex_lock(&mutex_ready);
    if (list_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_ready);
        return NULL;
    }

    int idx = 0;
    double max = calcular_respuesta(list_get(cola_ready, 0));
    for (int i = 1; i < list_size(cola_ready); ++i) {
        t_pcb* p = list_get(cola_ready, i);
        double rr = calcular_respuesta(p);
        if (rr > max) {
            max = rr;
            idx = i;
        }
    }

    t_pcb* elegido = list_remove(cola_ready, idx);
    pthread_mutex_unlock(&mutex_ready);
    return elegido;
}
