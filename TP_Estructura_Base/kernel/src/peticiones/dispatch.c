// dispatch.c
#include "dispatch.h"// asumimos funciones como crear_conexion, enviar_paquete, recibir_contexto
#include <commons/log.h>
#include <paquete/paquete.h>
#include <config/kernel_config.h>
#include <pcb/pcb.h>
#include <stdlib.h>
#include <mod_kernel.h>

extern t_log* logger;
extern t_kernel_config KCONF;
extern int socket_cpu_dispatch;

int enviar_proceso_a_cpu(t_pcb* pcb) {
    ///TODO: serializar_pcb_en_paquete(pcb);
    if (enviar_paquete(pcb, socket_cpu_dispatch) < -1) return -1;
    if (eliminar_paquete(pcb) < -1) return -1;
    return EXIT_SUCCESS;
}

int enviar_interrupt_cpu(uint32_t pid) {
    // conectar a puerto interrupt y enviar ID
    return 0;
}

int recibir_contexto_actualizado(int sock, t_pcb* pcb) {
    // recibir paquete y actualizar pcb fields
    return 0;
}
