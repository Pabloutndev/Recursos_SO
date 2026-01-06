
#include <conexiones/cpu.h>
#include <mod_kernel.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <pthread.h>

// Variables globales para sockets (definidas en mod_kernel.c o aquí)
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;

void conectar_cpu(char* ip, char* puerto_dispatch, char* puerto_interrupt) {
    socket_cpu_dispatch = crear_conexion(ip, puerto_dispatch);
    if (socket_cpu_dispatch < 0) {
        log_error(loggerError, "Fallo conexion CPU Dispatch");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Conectado a CPU Dispatch: %s:%s", ip, puerto_dispatch);
    
    // Handshake Dispatch
    handshake_cliente(socket_cpu_dispatch, HANDSHAKE_KERNEL, OK, logger); // CPU responde int

    socket_cpu_interrupt = crear_conexion(ip, puerto_interrupt);
    if (socket_cpu_interrupt < 0) {
        log_error(loggerError, "Fallo conexion CPU Interrupt");
        exit(EXIT_FAILURE);
    }
    log_info(logger, "Conectado a CPU Interrupt: %s:%s", ip, puerto_interrupt);

    // Handshake Interrupt
    handshake_cliente(socket_cpu_interrupt, HANDSHAKE_KERNEL, OK, logger);
}

void enviar_contexto_a_cpu(contexto_t* ctx) {
     // Implementar serializacion de Contexto
     // t_paquete* p = crear_paquete(PROCESO_EXEC);
     // ...
     // enviar_paquete(p, socket_cpu_dispatch);
}

void enviar_interrupcion_a_cpu(int pid, int motivo) {
    t_paquete* p = crear_paquete(INTERRUPCION_CPU);
    agregar_entero_a_paquete(p, pid);
    agregar_entero_a_paquete(p, motivo);
    enviar_paquete(p, socket_cpu_interrupt);
    eliminar_paquete(p);
}

contexto_t* recibir_contexto_de_cpu(void) {
     // ... recibir paquete en socket_cpu_dispatch (Bloqueante)
     // deserializar
     return NULL;
}
