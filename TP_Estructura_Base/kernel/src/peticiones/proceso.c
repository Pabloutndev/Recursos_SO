#include <peticiones/proceso.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/logger.h>
#include <mod_kernel.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <conexiones/memoria.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/mensajes.h>

extern int socket_memoria;
extern t_kernel_config KCONF;
extern int pid;
/* ===============================
 * COMMAND: START PROCESS (RUN)
 * 
 * Responsabilidades:
 * 1. Validar path del archivo de instrucciones
 * 2. Conectar a memoria si no está conectado
 * 3. Crear proceso en memoria (OP_MEM_INIT_PROCESO)
 * 4. Crear PCB con PID retornado
 * 5. Encolar en READY
 * 6. Señalizar planificador (sem_post)
 * =============================== */
void ejecutar_proceso(char* path)
{
    if (!path || strlen(path) == 0) {
        log_error(logger, "Kernel: Path de proceso inválido");
        return;
    }

    log_info(logger, "Kernel: Creación de Proceso solicitada - Path: %s", path);
    
    // Validar conexión con memoria
    if (socket_memoria < 0) {
        log_warning(logger, "Kernel: Reconectando con memoria...");
        conectar_memoria(KCONF.ip_memoria, KCONF.puerto_memoria);
        if (socket_memoria < 0) {
            log_error(logger, "Kernel: No se pudo conectar a memoria");
            return;
        }
    }

    // ✅ PASO 1: Crear proceso en memoria
    t_mem_init_proceso req;
    req.pid = pid;  // Memoria asignará PID
    strcpy(req.instrucciones, path);
    
    bool creado_en_memoria = kernel_init_proceso_path(&req);
    
    if (!creado_en_memoria) {
        log_error(logger, "Kernel: No se pudo crear proceso en memoria para %s", path);
        return;
    }

    // ✅ PASO 2: Crear PCB local
    t_pcb* pcb = pcb_crear(req.pid, KCONF.quantum);
    
    if (!pcb) {
        log_error(logger, "Kernel: No se pudo crear PCB para PID %u", req.pid);
        // TODO: Liberar proceso en memoria
        return;
    }

    // ✅ PASO 3: Encolar en READY
    pthread_mutex_lock(&mutex_ready);
    list_add(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_ready);
    
    log_info(logger, "Kernel: Proceso creado PID=%u, Path=%s", pcb->pid, path);
    
    // ✅ PASO 4: Señalizar planificador
    sem_post(&sem_hay_ready);
    
    /// NOTE: CERRAR??
    //liberar_conexion(socket_memoria);
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