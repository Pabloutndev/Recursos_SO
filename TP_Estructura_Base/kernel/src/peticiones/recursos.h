#ifndef RECURSOS_H_
#define RECURSOS_H_

#include <config/kernel_config.h>
#include <pcb/pcb.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>

typedef struct {
    char* nombre;
    int instancias;
    t_list* cola_bloqueados;
    pthread_mutex_t mutex; 
} t_recurso;

void recursos_init(t_kernel_config* config);
void recursos_destroy();

/* Retorna true si el proceso debe bloquearse, false si puede continuar */
bool recurso_wait(t_pcb* pcb, char* nombre_recurso);

/* Retorna un PCB desbloqueado si lo hubo, o NULL */
t_pcb* recurso_signal(char* nombre_recurso, t_pcb* pcb_signaler);

/* Obtener diccionario de recursos (para deteccion de deadlock) */
t_dictionary* recursos_obtener_diccionario(void);

/* Libera recursos bloqueados por un proceso al finalizar */
void recursos_liberar_proceso(uint32_t pid);

/* Libera recursos adquiridos por un proceso y desbloquea procesos en espera.
 * Retorna una lista de t_pcb* desbloqueados (el caller debe moverlos de BLOCKED a READY). */
t_list* recursos_liberar_adquiridos(t_pcb* pcb);

#endif
