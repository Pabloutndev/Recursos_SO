#include <pcb/pcb.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <string.h>
#include <config/kernel_config.h>
#include <mod_kernel.h>

static uint32_t PID_GLOBAL = 1;
extern t_kernel_config KCONF;

t_pcb* pcb_crear(int skt)
{
    t_pcb* pcb = malloc(sizeof(t_pcb));
    
    if (!pcb) return NULL;

    pcb->pid = generar_pid();
    pcb->quantum = KCONF.quantum;
    pcb->estado = NEW;
    
    pcb->program_counter = 0;
    
    pcb->instrucciones = list_create();
    pcb->instrucciones = list_create();
    
    // Registros inicializados en 0
    memset(&(pcb->registros), 0, sizeof(registros_t));

    pcb->tabla_segmentos = list_create();
    pcb->tam_proceso = 0;

    pcb->tabla_archivos = list_create();

    pcb->prioridad = 0;
    pcb->estimacion_rafaga = 0;
    pcb->tiempo_ready = temporal_create();
    
    pcb->socket_consola = skt;

    return pcb;
}

int generar_pid()
{
    return PID_GLOBAL++;
}

void pcb_destruir(t_pcb* pcb)
{
    if (!pcb) return;

    list_destroy(pcb->instrucciones);
    list_destroy(pcb->instrucciones);
    //list_destroy(pcb->registros); Estatica
    list_destroy(pcb->tabla_archivos);
    list_destroy(pcb->tabla_segmentos);

    if (pcb->tiempo_ready) {
        temporal_destroy(pcb->tiempo_ready);
    }

    free(pcb);
}
