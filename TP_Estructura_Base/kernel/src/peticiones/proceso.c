#include <peticiones/proceso.h>
#include <peticiones/ruta_procesos.h>
#include <planificacion/planificacion.h>
#include <pcb/pcb.h>
#include <loggers/logger.h>
#include <mod_kernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <adaptadores/kernel_memoria_adapter.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/mensajes.h>

// extern int socket_memoria;
// extern t_kernel_config KCONF;
// extern t_log* logger;
// extern t_log* loggerError;
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
void ejecutar_proceso(char* nombre_archivo)
{
    if (!nombre_archivo || strlen(nombre_archivo) == 0) {
        log_error(KERNEL_CTX.logger_error, "Proceso: nombre de archivo invalido");
        return;
    }

    // ✅ PASO 0: Normalizar nombre (agregar .txt si falta)
    char* nombre_proceso = construir_nombre_proceso(nombre_archivo);
    if (!nombre_proceso) {
        log_error(KERNEL_CTX.logger_error, "Proceso: no se pudo construir nombre");
        return;
    }

    // ✅ PASO 0.5: Validar que existe en la perspectiva de KERNEL
    if (!validar_existe_proceso_kernel(nombre_proceso)) {
        log_error(KERNEL_CTX.logger_error, "Proceso: archivo no encontrado %s", nombre_proceso);
        free(nombre_proceso);
        return;
    }

    log_info(KERNEL_CTX.logger, "Proceso: creacion solicitada %s", nombre_proceso);
    
    // ✅ PASO 1: Crear PCB (genera PID, estado NEW)
    t_pcb* pcb = pcb_crear();
    if (!pcb) {
        log_error(KERNEL_CTX.logger_error, "Proceso: error al crear PCB");
        free(nombre_proceso);
        return;
    }
    
    // ✅ PASO 2: Asignar nombre del proceso (Memoria lo usará para construir su propia ruta)
    pcb->path = nombre_proceso;  // Es el NOMBRE, no la ruta completa
    
    log_info(KERNEL_CTX.logger, "PID: %u - PCB creado, proceso=%s", pcb->pid, pcb->path);
    
    // ✅ PASO 3: Encolar en NEW (bajo lock)
    pthread_mutex_lock(&mutex_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_new);
    
    // ✅ PASO 4: Señalizar planificador largo plazo
    sem_post(&sem_hay_new);
    
    log_info(KERNEL_CTX.logger, "PID: %u - Encolado en NEW", pcb->pid);
}

/* ===============================
 * FINISH PROCESS (KILL)
 * =============================== */
void matar_proceso(int pid)
{
    log_info(KERNEL_CTX.logger, "PID: %d - Solicitud de finalizacion", pid);
    planificacion_finalizar_proceso(pid);
}

