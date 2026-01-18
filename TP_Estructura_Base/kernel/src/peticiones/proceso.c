#include <peticiones/proceso.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/logger.h>
#include <mod_kernel.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <adaptadores/kernel_memoria_adapter.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/mensajes.h>

extern int socket_memoria;
extern t_kernel_config KCONF;
extern t_log* logger;
extern t_log* loggerError;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_new;
extern sem_t sem_hay_ready;
extern sem_t sem_hay_new;
extern t_list* cola_ready;
extern t_list* cola_new;
/* ===============================
 * COMMAND: START PROCESS (RUN)
 * 
 * Responsabilidades:
 * 1. Validar path del archivo de instrucciones
 * 2. Crear PCB con estado NEW
 * 3. Encolar en NEW
 * 4. Señalizar planificador largo plazo
 * 5. El planificador largo plazo se encarga de hablar con Memoria
 * =============================== */
void ejecutar_proceso(char* path)
{
    if (!path || strlen(path) == 0) {
        log_error(loggerError, "Kernel: Path de proceso inválido");
        return;
    }

    log_info(logger, "Kernel: Creación de Proceso solicitada - Path: %s", path);
    
    // ✅ PASO 1: Crear PCB (genera PID, estado NEW)
    t_pcb* pcb = pcb_crear();
    if (!pcb) {
        log_error(loggerError, "Kernel: No se pudo crear PCB");
        return;
    }
    
    // ✅ PASO 2: Asignar path
    pcb->path = malloc(strlen(path) + 1);
    if (!pcb->path) {
        log_error(loggerError, "Kernel: No se pudo asignar memoria para path");
        pcb_destruir(pcb);
        return;
    }
    strcpy(pcb->path, path);
    
    log_info(logger, "Kernel: PCB creado - PID=%u, Path=%s", pcb->pid, path);
    
    // ✅ PASO 3: Encolar en NEW (bajo lock)
    pthread_mutex_lock(&mutex_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_new);
    
    // ✅ PASO 4: Señalizar planificador largo plazo
    sem_post(&sem_hay_new);
    
    log_info(logger, "Kernel: Proceso PID=%u encolado en NEW para inicialización", pcb->pid);
}

/* ===============================
 * FINISH PROCESS (KILL)
 * =============================== */
void matar_proceso(int pid)
{
    log_info(logger, "Solicitud de finalización del proceso %d", pid);
    planificacion_finalizar_proceso(pid);
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