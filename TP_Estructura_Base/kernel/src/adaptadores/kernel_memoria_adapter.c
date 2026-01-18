#include "kernel_memoria_adapter.h"
#include <protocolo/mensajes.h>
#include <protocolo/op_code.h>
#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <stdlib.h>
#include <commons/log.h>
#include <sys/socket.h>

extern int socket_memoria;
extern t_log* logger;
extern t_log* loggerError;

/* ========================================
 * TRANSFORMACIÓN DE ESTRUCTURAS
 * ======================================== */

t_mem_init_proceso* pcb_a_mem_init(t_pcb* pcb)
{
    t_mem_init_proceso* req = malloc(sizeof(t_mem_init_proceso));
    req->pid = pcb->pid;
    // El path se construye a partir del PID para este ejemplo
    snprintf(req->path, 256, "proceso_%u.txt", pcb->pid);
    return req;
}

t_mem_fin_proceso* pcb_a_mem_fin(uint32_t pid)
{
    t_mem_fin_proceso* req = malloc(sizeof(t_mem_fin_proceso));
    req->pid = pid;
    return req;
}

/* ========================================
 * OPERACIONES COMPLETAS (CON RESPUESTA)
 * ======================================== */

bool kernel_init_proceso(t_pcb* pcb)
{
    if (!pcb) {
        log_error(loggerError, "ADAPTER: PCB nulo en init_proceso");
        return false;
    }

    // Paso 1: Convertir PCB a estructura compartida
    t_mem_init_proceso* req = pcb_a_mem_init(pcb);

    // Paso 2: Enviar request a Memoria usando protocolo
    log_info(logger, "ADAPTER: Enviando OP_MEM_INIT_PROCESO (PID=%u)",
             req->pid);
    enviar_init_proceso(socket_memoria, req, OP_MEM_INIT_PROCESO);

    // Paso 3: Esperar respuesta (paquete con OP_OK o OP_FAIL)
    t_paquete* resp = recibir_paquete(socket_memoria);

    free(req);

    if (!resp) {
        log_error(loggerError, "ADAPTER: Error recibiendo respuesta de Memoria");
        return false;
    }

    bool exito = recibir_respuesta(resp);
    log_info(logger, "ADAPTER: Respuesta Memoria init_proceso: %s",
             exito ? "OK" : "FAIL");

    paquete_destroy(resp);
    return exito;
}

void kernel_fin_proceso(uint32_t pid)
{
    // Paso 1: Preparar estructura
    t_mem_fin_proceso req = {.pid = pid};

    // Paso 2: Enviar (one-way, no espera respuesta)
    log_info(logger, "ADAPTER: Enviando OP_MEM_FIN_PROCESO (PID=%u)", pid);
    enviar_fin_proceso(socket_memoria, &req, OP_MEM_FIN_PROCESO);
    // Memoria procesa y libera recursos, no responde
}

