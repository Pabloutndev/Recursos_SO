#ifndef MOD_CPU_H
#define MOD_CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <config/cpu_config.h>
#include <conexiones/cpu_kernel.h>
#include <conexiones/cpu_memoria.h>
#include <ciclo_instruccion/ciclo.h>
#include <interrupciones/interrupciones.h>
#include <registros/registros.h>
#include <loggers/logger.h>

#include <commons/log.h>
#include <commons/config.h>

extern t_log* logger;
extern t_log* loggerError;
extern t_cpu_config CPU_CONF;
extern t_contexto_cpu cpu_estado;

void cpu_set_motivo(t_cpu_motivo motivo);
t_cpu_motivo cpu_get_motivo(void);

void cpu_init(const char* path_config);
void cpu_run(void);
void cpu_shutdown(void);

#endif
