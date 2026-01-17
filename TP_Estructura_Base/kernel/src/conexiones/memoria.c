#include <conexiones/memoria.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <serializacion/serializacion.h>

extern int socket_memoria;

void conectar_memoria(char* ip, char* puerto) {
    socket_memoria = crear_conexion(ip, puerto);
     if (socket_memoria < 0) {
        log_error(loggerError, "Fallo conexion Memoria");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Conectado a Memoria: %s:%s", ip, puerto);

     // Handshake
    handshake_cliente(socket_memoria, OP_HANDSHAKE, OP_OK, logger);
}

// Need to include protocolo header

bool solicitar_creacion_proceso_memoria(uint32_t pid, int size) {
    t_mem_init_proceso req;
    req.pid = pid;
    req.tamanio = size;

    enviar_init_proceso(socket_memoria, &req, OP_MEM_INIT_PROCESO);

    // Esperar respuesta (paquete con OP_OK/OP_FAIL)
    t_paquete* resp = recibir_paquete(socket_memoria);
    if (!resp) return false;
    
    bool exito = recibir_respuesta(resp);
    paquete_destroy(resp);
    return exito;
}

void solicitar_fin_proceso_memoria(uint32_t pid) {
    t_mem_fin_proceso req;
    req.pid = pid;
    enviar_fin_proceso(socket_memoria, &req, OP_MEM_FIN_PROCESO);
}
