#include <peticiones/proceso.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/conexion.h>
#include <loggers/logger.h>
#include <mod_kernel.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <conexiones/memoria.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>

extern int socket_memoria;
extern t_kernel_config KCONF;

/* ===============================
 * COMMAND: START PROCESS (RUN)
 * =============================== */
void ejecutar_proceso(char* path)
{
    log_info(logger, "Creación de Proceso solicitada");
    
    // coneccion a memoria 
    if(socket_memoria < -1) {
        conectar_memoria(KCONF.ip_memoria, KCONF.puerto_memoria);
    }
    
    /// TODO: Crear proceso en memoria 
    t_paquete* paquete = crear_paquete(INIT_PROCESO);
    agregar_a_paquete(paquete, path, strlen(path) + 1);
    enviar_paquete(paquete, socket_memoria);

    int codop = recibir_operacion(socket_memoria);

    if (codop != OK) {
        log_error(logger, "No se pudo crear proceso en memoria");
        eliminar_paquete(paquete);
        liberar_conexion(socket_memoria);
        return;
    }

    //int pid = get_pid(socket_memoria);

    /* Crear PCB */
    //t_pcb* pcb = pcb_crear(pid, KCONF.quantum);
    //t_pcb* pcb = pcb_crear(socket_memoria);
    t_pcb* pcb = pcb_crear();

    /* Ingresar a NEW */
    ingresar_new(pcb);

    eliminar_paquete(paquete);
    
    //liberar_conexion(socket_memoria);
}

/* ===============================
 * FINISH PROCESS (KILL)
 * =============================== */
void matar_proceso(int pid)
{
    log_info(logger, "Solicitud de finalización del proceso %d", pid);
    
    t_pcb* pcb_encontrado = NULL;
    
    // Buscar en NEW
    pthread_mutex_lock(&mutex_new);
    for (int i = 0; i < list_size(cola_new); i++) {
        t_pcb* p = list_get(cola_new, i);
        if (p->pid == pid) {
            pcb_encontrado = p;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_new);
    
    // Buscar en READY
    if (!pcb_encontrado) {
        pthread_mutex_lock(&mutex_ready);
        for (int i = 0; i < list_size(cola_ready); i++) {
            t_pcb* p = list_get(cola_ready, i);
            if (p->pid == pid) {
                pcb_encontrado = p;
                break;
            }
        }
        pthread_mutex_unlock(&mutex_ready);
    }
    
    // Buscar en EXEC
    if (!pcb_encontrado) {
        pthread_mutex_lock(&mutex_exec);
        for (int i = 0; i < list_size(cola_exec); i++) {
            t_pcb* p = list_get(cola_exec, i);
            if (p->pid == pid) {
                pcb_encontrado = p;
                break;
            }
        }
        pthread_mutex_unlock(&mutex_exec);
    }
    
    // Buscar en BLOCKED
    if (!pcb_encontrado) {
        pthread_mutex_lock(&mutex_blocked);
        for (int i = 0; i < list_size(cola_blocked); i++) {
            t_pcb* p = list_get(cola_blocked, i);
            if (p->pid == pid) {
                pcb_encontrado = p;
                break;
            }
        }
        pthread_mutex_unlock(&mutex_blocked);
    }
    
    if (pcb_encontrado) {
        planificacion_matar_proceso(pcb_encontrado);
    } else {
        log_error(logger, "Proceso %d no encontrado", pid);
    }
    
    // TODO: Notificar a memoria para liberar recursos del proceso
}

/* ===============================
 * PS (mostrar estados)
 * =============================== */
void mostrar_procesos(void)
{
    listar_procesos_por_estado();
}

void initialize_process(t_pcb* p, int pid, int quantum) 
{
    p->estado = NEW;
    p->pid = pid;
    p->quantum = quantum;
    p->registros.AX = 0;
    p->registros.BX = 0;
    p->registros.CX = 0;
    p->registros.DX = 0;
    p->registros.EAX = 0;
    p->registros.EBX = 0;
    p->registros.ECX = 0;
    p->registros.EDX = 0;
    p->registros.SI = 0;
    p->registros.DI = 0;
    p->registros.PC = 0;
}
/*
t_list* get_listOfProcesses(char* name)
{
    int i = 0;
    // Modificar 6 por un valor calculado
    for (i=0; i < 6; i++)
    {
        if (strcmp(listsProcesses.listProcess[i].name, name) == 0)
        {
            return &(listsProcesses.listProcess[i]);
        }
    }
    return NULL;
}
*/