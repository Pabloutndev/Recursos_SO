
#include <logger/logger.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

// Helper to format PID list
char* lista_pids(t_list* cola) {
    char* string_pids = string_new();
    string_append(&string_pids, "[");

    for (int i = 0; i < list_size(cola); i++) {
        // Asumiendo que la cola tiene int* o PCB* con PID first.
        // Como es shared, asumimos que recibimos int* (PIDs) o struct procesada. 
        // El usuario pide "lista_pids(t_list* cola)".
        // Asumiré que es una lista de ints o punteros a int.
        
        // Cuidado: Si es lista de PCBs, esto fallará. 
        // PERO 'logger.c' en utils no conoce el PCB.
        // Asumo lista de PID (int*).
        int* pid_ptr = (int*)list_get(cola, i);
        if (pid_ptr != NULL) {
            char* pid_str = string_itoa(*pid_ptr);
            string_append(&string_pids, pid_str);
            free(pid_str);
            
            if (i < list_size(cola) - 1) {
                string_append(&string_pids, ", ");
            }
        }
    }
    string_append(&string_pids, "]");
    return string_pids;
}

void log_creacion_proceso(int pid) {
    log_info(logger, "Creacion de Proceso: PID: %d", pid);
}

void log_fin_proceso(int pid, const char* motivo) {
    log_info(logger, "Fin de Proceso: PID: %d - Motivo: %s", pid, motivo);
}

void log_cambio_estado(int pid, const char* ant, const char* act) {
    log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pid, ant, act);
}

void log_bloqueo(int pid, const char* motivo) {
    log_info(logger, "PID: %d - Bloqueado por: %s", pid, motivo);
}

void log_fin_quantum(int pid) {
    log_info(logger, "PID: %d - Fin de Quantum", pid);
}

void log_ingreso_ready(const char* algoritmo, t_list* cola) {
    char* pids = lista_pids(cola);
    log_info(logger, "Cola Ready %s: %s", algoritmo, pids);
    free(pids);
}

void log_wait(int pid, const char* recurso, int instancias) {
    log_info(logger, "PID: %d - WAIT: %s - Instancias: %d", pid, recurso, instancias);
}

void log_signal(int pid, const char* recurso, int instancias) {
    log_info(logger, "PID: %d - SIGNAL: %s - Instancias: %d", pid, recurso, instancias);
}

void log_page_fault(int pid, int pagina) {
    log_info(logger, "Page Fault: PID: %d - Pagina: %d", pid, pagina);
}

/* Planificación */
void log_inicio_planificacion(void) {
    log_info(logger, "Inicio de Planificacion");
}

void log_pausa_planificacion(void) {
    log_info(logger, "Pausa de Planificacion");
}

void log_cambio_multiprogramacion(int ant, int act) {
    log_info(logger, "Cambio de Grado de Multiprogramacion: %d -> %d", ant, act);
}

void log_inicio_deadlock(void) {
     log_info(logger, "Inicio de Analisis de Deadlock");
}

void log_deadlock_detectado(int pid, t_list* recursos, const char* req) {
    // Asumiendo recursos es lista de nombres (char*)
    char* lista_recursos = string_new();
    string_append(&lista_recursos, "[");
    for(int i=0; i<list_size(recursos); i++) {
        char* r = list_get(recursos, i);
        string_append(&lista_recursos, r);
         if (i < list_size(recursos) - 1) string_append(&lista_recursos, ", ");
    }
    string_append(&lista_recursos, "]");
    
    log_info(logger, "Deadlock Detectado: PID: %d - Recursos en posesion: %s - Recurso solicitado: %s", pid, lista_recursos, req);
    free(lista_recursos);
}
