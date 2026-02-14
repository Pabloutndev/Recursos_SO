#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <core/io_main.h>
#include <interfaces/dialfs.h>

void io_adapter_atender_sleep(int fd, t_paquete* p) {
    t_io_sleep* req = recibir_io_sleep(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: Iniciando SLEEP PID %u (%u ms)", req->pid, req->tiempo);
    usleep(req->tiempo * 1000);

    enviar_io_fin(fd, req->pid, true);
    free(req);
}

void io_adapter_atender_stdin_read(int fd, t_paquete* p) {
    t_io_stdin_read* req = recibir_io_stdin_read(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: STDIN_READ PID %u (dir_logica: %u, size: %u)",
             req->pid, req->direccion_logica, req->size);

    char* input = readline("> ");
    if (!input) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Error leyendo stdin para PID %u", req->pid);
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }

    // Truncar al tamanio solicitado
    uint32_t len = strlen(input);
    if (len > req->size) len = req->size;

    // Enviar escritura a Memoria
    t_mem_write mem_req = {
        .pid = req->pid,
        .direccion_logica = req->direccion_logica,
        .size = len,
        .buffer = input
    };
    enviar_escritura_memoria(IO_CTX.socket_memoria, &mem_req, OP_MEM_ESCRIBIR);

    // Esperar confirmacion de Memoria
    t_paquete* resp = recibir_paquete(IO_CTX.socket_memoria);
    if (!resp || !recibir_respuesta(resp)) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Memoria fallo al escribir para PID %u", req->pid);
        if (resp) paquete_destroy(resp);
        free(input);
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }
    paquete_destroy(resp);

    log_info(IO_CTX.logger, "IO_ADAPTER: STDIN_READ PID %u completado (%u bytes escritos en memoria)", req->pid, len);

    free(input);
    enviar_io_fin(fd, req->pid, true);
    free(req);
}

void io_adapter_atender_stdout_write(int fd, t_paquete* p) {
    t_io_stdout_write* req = recibir_io_stdout_write(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: STDOUT_WRITE PID %u (dir_logica: %u, size: %u)",
             req->pid, req->direccion_logica, req->size);

    // Solicitar lectura a Memoria
    t_mem_read mem_req = {
        .pid = req->pid,
        .direccion_logica = req->direccion_logica,
        .size = req->size
    };
    enviar_lectura_memoria(IO_CTX.socket_memoria, &mem_req, OP_MEM_LEER);

    // Recibir respuesta con los datos
    t_paquete* resp = recibir_paquete(IO_CTX.socket_memoria);
    if (!resp) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Memoria desconectada al leer para PID %u", req->pid);
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }

    t_mem_respuesta_lectura* lectura = recibir_respuesta_lectura(resp);
    paquete_destroy(resp);

    if (!lectura || !lectura->ok) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Memoria fallo al leer para PID %u", req->pid);
        if (lectura) {
            if (lectura->data) free(lectura->data);
            free(lectura);
        }
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }

    // Imprimir los datos por consola
    printf("%.*s\n", lectura->size, (char*)lectura->data);

    log_info(IO_CTX.logger, "IO_ADAPTER: STDOUT_WRITE PID %u completado (%u bytes)", req->pid, lectura->size);

    free(lectura->data);
    free(lectura);
    enviar_io_fin(fd, req->pid, true);
    free(req);
}

void io_adapter_atender_fs_create(int fd, t_paquete* p) {
    t_io_fs_create* req = recibir_io_fs_create(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: Iniciando FS_CREATE PID %u (Archivo: %s)", req->pid, req->path);
    bool ok = io_dialfs_create(req->path);

    enviar_io_fin(fd, req->pid, ok);
    free(req);
}

void io_adapter_atender_fs_delete(int fd, t_paquete* p) {
    t_io_fs_create* req = recibir_io_fs_create(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: Iniciando FS_DELETE PID %u (Archivo: %s)", req->pid, req->path);
    bool ok = io_dialfs_delete(req->path);

    enviar_io_fin(fd, req->pid, ok);
    free(req);
}

void io_adapter_atender_fs_truncate(int fd, t_paquete* p) {
    t_io_fs_write* req = recibir_io_fs_write(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: Iniciando FS_TRUNCATE PID %u (Archivo: %s, Nuevo Tam: %u)", req->pid, req->path, req->size);
    bool ok = io_dialfs_truncate(req->path, req->size);

    enviar_io_fin(fd, req->pid, ok);
    free(req);
}

void io_adapter_atender_fs_write(int fd, t_paquete* p) {
    t_io_fs_write* req = recibir_io_fs_write(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: Iniciando FS_WRITE PID %u (Archivo: %s, Tam: %u, Offset: %u, Dir_Logica: %u)",
             req->pid, req->path, req->size, req->offset, req->direccion_logica);

    // Simular tiempo de trabajo
    usleep(IO_CTX.config->tiempo_unidad_trabajo * 1000);

    // 1. Leer datos de Memoria (la direccion logica del proceso)
    t_mem_read mem_req = {
        .pid = req->pid,
        .direccion_logica = req->direccion_logica,
        .size = req->size
    };
    enviar_lectura_memoria(IO_CTX.socket_memoria, &mem_req, OP_MEM_LEER);

    // 2. Recibir respuesta con los datos desde Memoria
    t_paquete* resp = recibir_paquete(IO_CTX.socket_memoria);
    if (!resp) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Memoria desconectada al leer para FS_WRITE PID %u", req->pid);
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }

    t_mem_respuesta_lectura* lectura = recibir_respuesta_lectura(resp);
    paquete_destroy(resp);

    if (!lectura || !lectura->ok) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Memoria fallo al leer datos para FS_WRITE PID %u", req->pid);
        if (lectura) {
            if (lectura->data) free(lectura->data);
            free(lectura);
        }
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }

    // 3. Escribir los datos leidos de memoria en el archivo DIALFS
    bool ok = io_dialfs_write(req->path, lectura->data, lectura->size, req->offset);

    log_info(IO_CTX.logger, "IO_ADAPTER: FS_WRITE PID %u %s (%u bytes)", req->pid, ok ? "completado" : "fallido", lectura->size);

    free(lectura->data);
    free(lectura);
    enviar_io_fin(fd, req->pid, ok);
    free(req);
}

void io_adapter_atender_fs_read(int fd, t_paquete* p) {
    t_io_fs_write* req = recibir_io_fs_write(p);
    if (!req) return;

    log_info(IO_CTX.logger, "IO_ADAPTER: Iniciando FS_READ PID %u (Archivo: %s, Tam: %u, Offset: %u, Dir_Logica: %u)",
             req->pid, req->path, req->size, req->offset, req->direccion_logica);

    // Simular tiempo de trabajo
    usleep(IO_CTX.config->tiempo_unidad_trabajo * 1000);

    // 1. Leer datos del archivo DIALFS
    void* buffer = malloc(req->size);
    if (!buffer) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Error de memoria al asignar buffer para FS_READ PID %u", req->pid);
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }

    bool ok = io_dialfs_read(req->path, buffer, req->size, req->offset);
    if (!ok) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Error leyendo archivo DIALFS para FS_READ PID %u", req->pid);
        free(buffer);
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }

    // 2. Escribir datos leidos del archivo en Memoria del proceso
    t_mem_write mem_req = {
        .pid = req->pid,
        .direccion_logica = req->direccion_logica,
        .size = req->size,
        .buffer = buffer
    };
    enviar_escritura_memoria(IO_CTX.socket_memoria, &mem_req, OP_MEM_ESCRIBIR);

    // 3. Esperar confirmacion de Memoria
    t_paquete* resp = recibir_paquete(IO_CTX.socket_memoria);
    if (!resp || !recibir_respuesta(resp)) {
        log_error(IO_CTX.logger, "IO_ADAPTER: Memoria fallo al escribir datos para FS_READ PID %u", req->pid);
        if (resp) paquete_destroy(resp);
        free(buffer);
        enviar_io_fin(fd, req->pid, false);
        free(req);
        return;
    }
    paquete_destroy(resp);

    log_info(IO_CTX.logger, "IO_ADAPTER: FS_READ PID %u completado (%u bytes)", req->pid, req->size);

    free(buffer);
    enviar_io_fin(fd, req->pid, true);
    free(req);
}
