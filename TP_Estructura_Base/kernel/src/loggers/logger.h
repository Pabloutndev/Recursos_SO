#ifndef LOGGER_H
#define LOGGER_H
#include <commons/log.h>

extern t_log* logger;
extern t_log* loggerError;

///NOTE: Logs obligatorios

///NOTE: Procesos (PCBs)

void log_creacion_proceso(int pid);
void log_fin_proceso(int pid, const char* motivo);
void log_cambio_estado(int pid, const char* ant, const char* act);
void log_bloqueo(int pid, const char* motivo);
void log_fin_quantum(int pid);
void log_ingreso_ready(const char* algoritmo, t_list* cola);
void log_wait(int pid, const char* recurso, int instancias);
void log_signal(int pid, const char* recurso, int instancias);
void log_page_fault(int pid, int pagina);

///NOTE: Planificación

void log_inicio_planificacion(void);
void log_pausa_planificacion(void);
void log_cambio_multiprogramacion(int ant, int act);
void log_inicio_deadlock(void);
void log_deadlock_detectado(int pid, t_list* recursos, const char* req);

//NOTE: Planificacion - PIDs
char* lista_pids(t_list* cola);

#endif /* LOGGER_H */

