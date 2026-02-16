#include <adaptadores/kernel_io_adapter.h>
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <serializacion/serializacion.h>
#include <paquete/paquete.h>
#include <conexion/conexion.h>
#include <loggers/logger.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

extern t_log* logger;
extern t_log* loggerError;

// Forward declaration: función que obtiene socket de interfaz
// Debe ser implementada en conexiones/io.c
extern int obtener_socket_interfaz(const char* nombre_interfaz);

void kernel_fs_operation(t_pcb* pcb,
                                     const char* tipo_operacion,
                                     const char* path,
                                     uint32_t tamanio,
                                     char* interfaz_io)
{
    if (!pcb || !tipo_operacion || !interfaz_io) {
        log_error(loggerError, "FS error: parametros nulos");
        return;
    }

    // Obtener socket de la interfaz
    int socket_io = obtener_socket_interfaz(interfaz_io);
    if (socket_io < 0) {
        log_error(loggerError, "PID: %u - FS error: interfaz '%s' no encontrada", pcb->pid, interfaz_io);
        return;
    }

    // Mapear tipo de operación a op_code y enviar estructura correspondiente

    if (strcmp(tipo_operacion, "CREATE") == 0) {
        t_io_fs_create* req = malloc(sizeof(t_io_fs_create));
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);

        log_info(logger, "PID: %u - FS_CREATE %s -> %s",
                 pcb->pid, path, interfaz_io);

        enviar_io_fs_create(socket_io, req);

        free(req);
    }
    else if (strcmp(tipo_operacion, "DELETE") == 0) {
        t_io_fs_create* req = malloc(sizeof(t_io_fs_create)); // Mismo struct basta para DELETE
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);

        log_info(logger, "PID: %u - FS_DELETE %s -> %s",
                 pcb->pid, path, interfaz_io);

        t_paquete* p = serializar_io_fs_create(req);
        p->codigo_operacion = OP_IO_FS_DELETE;
        enviar_paquete(socket_io, p);
        paquete_destroy(p);

        free(req);
    }
    else if (strcmp(tipo_operacion, "WRITE") == 0) {
        t_io_fs_write* req = malloc(sizeof(t_io_fs_write));
        req->pid = pcb->pid;
        snprintf(req->path, sizeof(req->path), "%s", path);
        req->size = tamanio;

        log_info(logger, "PID: %u - FS_WRITE %s tam=%u -> %s",
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

        log_info(logger, "PID: %u - FS_READ %s tam=%u -> %s",
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

        log_info(logger, "PID: %u - FS_TRUNCATE %s tam=%u -> %s",
                 pcb->pid, path, tamanio, interfaz_io);

        t_paquete* p = serializar_io_fs_write(req);
        p->codigo_operacion = OP_IO_FS_TRUNCATE;
        enviar_paquete(socket_io, p);
        paquete_destroy(p);

        free(req);
    }
    else {
        log_warning(logger, "PID: %u - FS operacion desconocida: %s", pcb->pid, tipo_operacion);
        return;
    }
}
