#include "kernel_io_adapter.h"
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <serializacion/serializacion.h>
#include <paquete/paquete.h>
#include <conexion/conexion.h>
#include <planificacion/planificacion.h>
#include <loggers/logger.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <stdlib.h>
#include <string.h>

extern t_log* logger;
extern t_log* loggerError;

// Forward declaration: función que obtiene socket de interfaz
// Debe ser implementada en conexiones/io.c
extern int obtener_socket_interfaz(const char* nombre_interfaz);

/* ========================================
 * TRANSFORMACIÓN DE ESTRUCTURAS
 * ======================================== */

t_io_sleep* pcb_a_io_sleep(t_pcb* pcb, uint32_t tiempo)
{
    t_io_sleep* io = malloc(sizeof(t_io_sleep));
    io->pid = pcb->pid;
    io->tiempo = tiempo;
    return io;
}

/* ========================================
 * OPERACIONES COMPLETAS
 * ======================================== */

void kernel_sleep(t_pcb* pcb, uint32_t tiempo_ms,
                              char* interfaz_io)
{
    if (!pcb || !interfaz_io) {
        log_error(loggerError, "IO_SLEEP error: parametros nulos");
        return;
    }

    // Paso 1: Convertir a estructura compartida
    t_io_sleep* io_req = pcb_a_io_sleep(pcb, tiempo_ms);

    // Paso 2: Obtener socket de la interfaz
    int socket_io = obtener_socket_interfaz(interfaz_io);
    if (socket_io < 0) {
        log_error(loggerError, "PID: %u - IO_SLEEP error: interfaz '%s' no encontrada", pcb->pid, interfaz_io);
        free(io_req);
        return;
    }

    // Paso 3: Enviar usando protocolo
    log_info(logger, "PID: %u - IO_SLEEP %u ms -> %s",
             io_req->pid, io_req->tiempo, interfaz_io);
    enviar_io_sleep(socket_io, io_req);

    // NOTA: IO ejecutará sleep y luego enviará OP_IO_FIN_OPERACION
    // Eso se maneja en el servidor de IO (conexiones/io.c - handler_io_connection)
    // Kernel escucha en ese handler y desbloquea el proceso cuando reciba FIN

    free(io_req);
}

void kernel_io_adapter_atender_fin_operacion(int fd, t_paquete* p)
{
    uint32_t pid = recibir_pid_fin_io(p);
    log_info(logger, "PID: %u - IO fin operacion", pid);

    // Buscar el PCB en cola_blocked y moverlo a cola_ready
    t_pcb* pcb = NULL;

    pthread_mutex_lock(&mutex_blocked);
    for (int i = 0; i < list_size(cola_blocked); i++) {
        t_pcb* aux = (t_pcb*) list_get(cola_blocked, i);
        if (aux && aux->pid == pid) {
            pcb = aux;
            list_remove(cola_blocked, i);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_blocked);

    if (!pcb) {
        log_error(loggerError, "PID: %u - IO fin error: no encontrado en BLOCKED", pid);
        return;
    }

    // Cambiar estado a READY y encolar
    // VRR: si quantum_restante == 0, resetear al quantum completo
    if (pcb->quantum_restante <= 0) {
        pcb->quantum_restante = pcb->quantum;
    }
    pcb->estado = READY;
    pcb->tiempo_ready = temporal_create();

    pthread_mutex_lock(&mutex_ready);
    list_add(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_ready);

    sem_post(&sem_hay_ready);

    log_cambio_estado(pid, "BLOCKED", "READY");
}
