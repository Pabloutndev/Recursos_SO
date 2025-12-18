#include <peticiones/proceso.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/conexion.h>
#include <mod_kernel.h>
#include <stdlib.h>
#include <string.h>

extern t_log* logger;
extern t_kernel_config KCONF;

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
    log_info(logger, "Finaliza el proceso %d - Motivo: SUCCESS", pid);
/*
    int socket_mem = crear_socket(
        logger,
        CLIENTE,
        KCONF.ip_memoria,
        KCONF.puerto_memoria
    );

    t_paquete* paquete = crear_paquete_con_codigo_op(PCKT_FINISH_PROCESS);
    agregar_entero_a_paquete(paquete, pid);
    enviar_paquete(paquete, socket_mem);

    eliminar_paquete(paquete);
    liberar_conexion(socket_mem);*/

    /* El retiro real de colas lo hace planificación */
}

/* ===============================
 * PS (mostrar estados)
 * =============================== */
void mostrar_procesos(void)
{
    //listar_procesos_por_estado();
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