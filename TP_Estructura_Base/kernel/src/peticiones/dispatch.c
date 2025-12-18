// dispatch.c
#include "dispatch.h"// asumimos funciones como crear_conexion, enviar_paquete, recibir_contexto
#include <commons/log.h>
#include <config/kernel_config.h>
#include <pcb/pcb.h>

extern t_log* logger;
extern t_kernel_config KCONF;

int enviar_proceso_a_cpu(t_pcb* pcb) {
    // crear socket, usar utils para serializar proceso y enviar
    // ejemplo (pseudocódigo):
    // int sock = crear_socket_cliente(KCONF.ip_cpu, KCONF.puerto_cpu_dispatch);
    // t_paquete* p = serializar_pcb_en_paquete(pcb);
    // enviar_paquete(p, sock);
    // eliminar_paquete(p);
    // return sock;
    return -1;
}

int enviar_interrupt_cpu(uint32_t pid) {
    // conectar a puerto interrupt y enviar ID
    return 0;
}

int recibir_contexto_actualizado(int sock, t_pcb* pcb) {
    // recibir paquete y actualizar pcb fields
    return 0;
}
