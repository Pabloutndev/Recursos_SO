#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <unistd.h>
#include <stdlib.h>
#include <commons/log.h>

extern t_log* logger;

void io_adapter_atender_sleep(int fd, t_paquete* p) {
    t_io_sleep* req = recibir_io_sleep(p);
    if (!req) return;

    log_info(logger, "IO_ADAPTER: Iniciando SLEEP PID %u (%u ms)", req->pid, req->tiempo);
    usleep(req->tiempo * 1000);
    
    enviar_io_fin(fd, req->pid, true);
    free(req);
}

void io_adapter_atender_fs_create(int fd, t_paquete* p) {
    t_io_fs_create* req = recibir_io_fs_create(p);
    if (!req) return;

    log_info(logger, "IO_ADAPTER: Iniciando FS_CREATE PID %u (Archivo: %s)", req->pid, req->path);
    // TODO: dialfs_create(req->path);
    
    enviar_io_fin(fd, req->pid, true);
    free(req->path);
    free(req);
}

void io_adapter_atender_fs_delete(int fd, t_paquete* p) {
    // Similar a create
    enviar_respuesta_ok(fd);
}

void io_adapter_atender_fs_truncate(int fd, t_paquete* p) {
    enviar_respuesta_ok(fd);
}

void io_adapter_atender_fs_write(int fd, t_paquete* p) {
    t_io_fs_write* req = recibir_io_fs_write(p);
    if (!req) return;

    log_info(logger, "IO_ADAPTER: Iniciando FS_WRITE PID %u (Archivo: %s, Tam: %u)", req->pid, req->path, req->size);
    // TODO: dialfs_write(...)
    
    enviar_io_fin(fd, req->pid, true);
    free(req->path);
    free(req);
}

void io_adapter_atender_fs_read(int fd, t_paquete* p) {
    enviar_respuesta_ok(fd);
}
