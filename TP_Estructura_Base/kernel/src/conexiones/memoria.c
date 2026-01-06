
#include <conexiones/memoria.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>

extern int socket_memoria;

void conectar_memoria(char* ip, char* puerto) {
    socket_memoria = crear_conexion(ip, puerto);
     if (socket_memoria < 0) {
        log_error(loggerError, "Fallo conexion Memoria");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Conectado a Memoria: %s:%s", ip, puerto);

     // Handshake
    handshake_cliente(socket_memoria, HANDSHAKE_KERNEL, OK, logger);
}

bool solicitar_creacion_proceso_memoria(uint32_t pid, int size) {
    t_paquete* p = crear_paquete(INIT_PROCESO);
    agregar_entero_a_paquete(p, pid);
    agregar_entero_a_paquete(p, size);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);

    // Esperar respuesta (OK/FAIL)
    int resp;
    recv(socket_memoria, &resp, sizeof(int), MSG_WAITALL);
    return resp == OK;
}

void solicitar_fin_proceso_memoria(uint32_t pid) {
    t_paquete* p = crear_paquete(FIN_PROCESO);
    agregar_entero_a_paquete(p, pid);
    enviar_paquete(p, socket_memoria);
    eliminar_paquete(p);
}
