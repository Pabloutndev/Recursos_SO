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

// Las funciones de solicitud ahora están en adaptadores/kernel_memoria_adapter.c
