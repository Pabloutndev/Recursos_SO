#include <conexiones/memoria.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

extern int socket_memoria;

void conectar_memoria(char* ip, char* puerto) {
    socket_memoria = crear_conexion(ip, puerto);
     if (socket_memoria < 0) {
        log_error(loggerError, "Fallo conexion Memoria");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Conectado a Memoria: %s:%s", ip, puerto);

     // Handshake
    handshake_cliente(socket_memoria, OP_HANDSHAKE_KERNEL, OP_OK, logger);
}

// Need to include protocolo header
#include <protocolo/memoria.h>
#include <serializacion/memoria.h>

bool solicitar_creacion_proceso_memoria(uint32_t pid, int size) {
    t_mem_init_proceso req;
    req.pid = pid;
    req.size = size;

    enviar_init_proceso(socket_memoria, &req);

    // Esperar respuesta (OK/FAIL)
    int resp = OP_FAIL;
    recv(socket_memoria, &resp, sizeof(int), MSG_WAITALL);
    return resp == OP_OK;
}

void solicitar_fin_proceso_memoria(uint32_t pid) {
    t_mem_fin_proceso req;
    req.pid = pid;
    enviar_fin_proceso(socket_memoria, &req);
}
