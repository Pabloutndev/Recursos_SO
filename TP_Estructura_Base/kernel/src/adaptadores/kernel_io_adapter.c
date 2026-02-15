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
        log_error(loggerError, "IO_ADAPTER: Parámetros nulos en sleep");
        return;
    }

    // Paso 1: Convertir a estructura compartida
    t_io_sleep* io_req = pcb_a_io_sleep(pcb, tiempo_ms);

    // Paso 2: Obtener socket de la interfaz
    int socket_io = obtener_socket_interfaz(interfaz_io);
    if (socket_io < 0) {
        log_error(loggerError, "IO_ADAPTER: Interfaz IO '%s' no encontrada", interfaz_io);
        free(io_req);
        return;
    }

    // Paso 3: Enviar usando protocolo
    log_info(logger, "IO_ADAPTER: Enviando OP_IO_SLEEP (PID=%u, TIEMPO=%u ms) a %s",
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
    log_info(logger, "ADAPTER: IO notifica FIN_OPERACION para PID %u", pid);

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
        log_error(loggerError, "ADAPTER: FIN_IO - PID %u no encontrado en BLOCKED", pid);
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

void kernel_fs_operation(t_pcb* pcb, 
                                     const char* tipo_operacion,
                                     const char* path,
                                     uint32_t tamanio,
                                     char* interfaz_io)
{
    if (!pcb || !tipo_operacion || !interfaz_io) {
        log_error(loggerError, "IO_ADAPTER: Parámetros nulos en fs_operation");
        return;
    }

    // Obtener socket de la interfaz
    int socket_io = obtener_socket_interfaz(interfaz_io);
    if (socket_io < 0) {
        log_error(loggerError, "IO_ADAPTER: Interfaz IO '%s' no encontrada", interfaz_io);
        return;
    }

    // Mapear tipo de operación a op_code y enviar estructura correspondiente
    
    if (strcmp(tipo_operacion, "CREATE") == 0) {
        t_io_fs_create* req = malloc(sizeof(t_io_fs_create));
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);
        
        log_info(logger, "IO_ADAPTER: Enviando OP_IO_FS_CREATE (PID=%u, ARCHIVO=%s) a %s",
                 pcb->pid, path, interfaz_io);
        
        enviar_io_fs_create(socket_io, req);
        
        free(req);
    }
    else if (strcmp(tipo_operacion, "DELETE") == 0) {
        t_io_fs_create* req = malloc(sizeof(t_io_fs_create)); // Mismo struct basta para DELETE
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);

        log_info(logger, "IO_ADAPTER: Enviando OP_IO_FS_DELETE (PID=%u, ARCHIVO=%s) a %s",
                 pcb->pid, path, interfaz_io);
        
        enviar_respuesta(socket_io, OP_IO_FS_DELETE); // O implementar enviar_io_fs_delete
        // Por consistencia implementamos enviar_io_fs_create con el opcode correcto
        t_paquete* p = serializar_io_fs_create(req);
        p->codigo_operacion = OP_IO_FS_DELETE;
        enviar_paquete(socket_io, p);
        paquete_destroy(p);

        free(req);
    }
    else if (strcmp(tipo_operacion, "READ") == 0) {
        log_info(logger, "IO_ADAPTER: Enviando OP_IO_FS_READ (PID=%u, ARCHIVO=%s) a %s",
                 pcb->pid, path, interfaz_io);
        
        // TODO: Implementar estructura y protocolo para READ
        log_warning(logger, "IO_ADAPTER: FS_READ no completamente implementado");
    }
    else if (strcmp(tipo_operacion, "WRITE") == 0) {
        t_io_fs_write* req = malloc(sizeof(t_io_fs_write));
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);
        req->size = tamanio;
        
        log_info(logger, "IO_ADAPTER: Enviando OP_IO_FS_WRITE (PID=%u, ARCHIVO=%s, TAM=%u) a %s",
                 pcb->pid, path, tamanio, interfaz_io);
        
        enviar_io_fs_write(socket_io, req);
        
        free(req);
    }
    else if (strcmp(tipo_operacion, "READ") == 0) {
        t_io_fs_write* req = malloc(sizeof(t_io_fs_write)); // Reusamos struct write para read (pid, path, offset, size)
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);
        req->size = tamanio;
        req->offset = 0; // O el que corresponda

        log_info(logger, "IO_ADAPTER: Enviando OP_IO_FS_READ (PID=%u, ARCHIVO=%s, TAM=%u) a %s",
                 pcb->pid, path, tamanio, interfaz_io);
        
        t_paquete* p = serializar_io_fs_write(req);
        p->codigo_operacion = OP_IO_FS_READ;
        enviar_paquete(socket_io, p);
        paquete_destroy(p);

        free(req);
    }
    else if (strcmp(tipo_operacion, "TRUNCATE") == 0) {
        t_io_fs_write* req = malloc(sizeof(t_io_fs_write));
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);
        req->size = tamanio; // Nuevo tamaño

        log_info(logger, "IO_ADAPTER: Enviando OP_IO_FS_TRUNCATE (PID=%u, ARCHIVO=%s, NUEVO_TAM=%u) a %s",
                 pcb->pid, path, tamanio, interfaz_io);
        
        t_paquete* p = serializar_io_fs_write(req);
        p->codigo_operacion = OP_IO_FS_TRUNCATE;
        enviar_paquete(socket_io, p);
        paquete_destroy(p);

        free(req);
    }
    else {
        log_warning(logger, "IO_ADAPTER: Operación FS desconocida: %s", tipo_operacion);
        return;
    }
}


