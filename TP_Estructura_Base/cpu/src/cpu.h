#ifndef MOD_CPU_H
#define MOD_CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <config/cpu_config.h>
#include <conexiones/cpu_memoria.h>
#include <ciclo_instruccion/ciclo.h>
#include <interrupciones/interrupciones.h>
#include <registros/registros.h>
#include <loggers/logger.h>

#include <commons/log.h>
#include <commons/config.h>

typedef struct {
    t_log* logger;
    t_log* logger_error;
    t_cpu_config config;
    t_contexto_cpu estado;
    int socket_dispatch;
    int socket_interrupt;
    int socket_memoria;
    t_motivo_desalojo motivo_desalojo;
} t_cpu_context;

extern t_cpu_context CPU_CTX;

void cpu_init(const char* path_config);
void cpu_run(void);
void cpu_shutdown(void);

#endif