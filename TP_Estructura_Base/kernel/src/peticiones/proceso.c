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

/* ===============================
 * START PROCESS (RUN)
 * =============================== */
void ejecutar_proceso(char* path)
{
    log_info(logger, "Creación de Proceso solicitada");

    /* Conexión a memoria */
    /*int socket_mem = crear_socket(
        logger,
        CLIENTE,
        KCONF.ip_memoria,
        KCONF.puerto_memoria
    );

    t_paquete* paquete = crear_paquete_con_codigo_op(PCKT_START_PROCESS);
    agregar_a_paquete(paquete, path, strlen(path) + 1);
    enviar_paquete(paquete, socket_mem);

    int codop = recibir_operacion(socket_mem);

    if (codop != OK) {
        log_error(logger, "No se pudo crear proceso en memoria");
        eliminar_paquete(paquete);
        liberar_conexion(socket_mem);
        return;
    }

    //int pid = get_pid(socket_mem);

    /* Crear PCB */
    //t_pcb* pcb = pcb_crear(pid, KCONF.quantum);
    int socket_mem = 1;
    t_pcb* pcb = pcb_crear(socket_mem);

    /* Ingresar a NEW */
    ingresar_new(pcb);
/*
    eliminar_paquete(paquete);
    liberar_conexion(socket_mem);*/
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


/*
void initialize_process(t_process* process, int pid, int quantum) 
{
    process->PID = pid;
    process->QUANTUM = quantum;
    process->cpu.AX = 0;
    process->cpu.BX = 0;
    process->cpu.CX = 0;
    process->cpu.DX = 0;
    process->cpu.EAX = 0;
    process->cpu.EBX = 0;
    process->cpu.ECX = 0;
    process->cpu.EDX = 0;
    process->cpu.SI = 0;
    process->cpu.DI = 0;
    process->cpu.PC = 0;
}

t_listProcess* get_listOfProcesses(char* name)
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