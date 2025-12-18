#include <loggers/logger.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcb/pcb.h>
#include <mod_kernel.h>

extern t_log* logger;

///NOTE: Logs obligatorios

void log_creacion_proceso(int pid) {
    log_info(logger, "Se crea el proceso %d en NEW", pid);
}

void log_fin_proceso(int pid, const char* motivo) {
    log_info(logger, "Finaliza el proceso %d - Motivo: %s", pid, motivo);
}

void log_cambio_estado(int pid, const char* ant, const char* act) {
    log_info(logger,
        "PID: %d - Estado Anterior: %s - Estado Actual: %s",
        pid, ant, act);
}

void log_bloqueo(int pid, const char* motivo) {
    log_info(logger,
        "PID: %d - Bloqueado por: %s",
        pid, motivo);
}

void log_fin_quantum(int pid) {
    log_info(logger,
        "PID: %d - Desalojado por fin de Quantum",
        pid);
}

void log_ingreso_ready(const char* algoritmo, t_list* cola) {
    char* pids = string_new();
    for (int i = 0; i < list_size(cola); i++) {
        t_pcb* pcb = list_get(cola, i);
        string_append_with_format(&pids, "%d ", pcb->pid);
    }
    log_info(logger,
        "Cola Ready %s: [%s]",
        algoritmo, pids);
    free(pids);
}

void log_wait(int pid, const char* recurso, int instancias) {
    log_info(logger,
        "PID: %d - Wait: %s - Instancias: %d",
        pid, recurso, instancias);
}

void log_signal(int pid, const char* recurso, int instancias) {
    log_info(logger,
        "PID: %d - Signal: %s - Instancias: %d",
        pid, recurso, instancias);
}

void log_page_fault(int pid, int pagina) {
    log_info(logger,
        "Page Fault PID: %d - Pagina: %d",
        pid, pagina);
}

///NOTE: Planificación

void log_inicio_planificacion(void) {
    log_info(logger, "INICIO DE PLANIFICACIÓN");
}

void log_pausa_planificacion(void) {
    log_info(logger, "PAUSA DE PLANIFICACIÓN");
}

void log_cambio_multiprogramacion(int ant, int act) {
    log_info(logger,
        "Grado Anterior: %d - Grado Actual: %d",
        ant, act);
}

///NOTE: Deadlock

void log_inicio_deadlock(void) {
    log_info(logger, "ANÁLISIS DE DETECCIÓN DE DEADLOCK");
}

void log_deadlock_detectado(int pid, t_list* recursos, const char* req) {
    char* res = string_new();
    for (int i = 0; i < list_size(recursos); i++) {
        string_append_with_format(&res, "%s ",
            (char*) list_get(recursos, i));
    }

    log_info(logger,
        "Deadlock detectado: %d - Recursos en posesión %s - Recurso requerido: %s",
        pid, res, req);

    free(res);
}

char* lista_pids(t_list* cola)
{
    char* resultado = string_new();

    void _append_pid(void* elem) {
        t_pcb* pcb = (t_pcb*) elem;
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%u,", pcb->pid);
        string_append(&resultado, pid_str);
    }

    list_iterate(cola, _append_pid);

    // Eliminar coma final
    int len = strlen(resultado);
    if (len > 0)
        resultado[len - 1] = '\0';

    return resultado; // ⚠️ el caller debe free()
}
