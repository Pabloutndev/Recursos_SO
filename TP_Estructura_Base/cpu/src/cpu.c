#include <cpu.h>
#include <config/cpu_config.h>
#include <loggers/logger.h>
#include <registros/registros.h>
#include <interrupciones/interrupciones.h>
#include <adaptadores/contexto_cpu_adapter.h>
#include <protocolo/mensajes.h>
#include <tlb/tlb.h>
#include <server/cpu_server.h>
#include <conexiones/cpu_memoria.h>
#include <unistd.h>
#include <string.h>

/// Contexto Global del Modulo
t_cpu_context CPU_CTX;

// Globals usados por extern en sub-módulos (apuntan a CPU_CTX)
t_log* logger = NULL;
t_log* loggerError = NULL;
int socket_dispatch = -1;
int socket_interrupt = -1;
t_contexto_cpu cpu_estado;
t_motivo_desalojo motivo_desalojo;
instruccion_t ultima_instruccion;

void cpu_init(const char* path_config)
{
    CPU_CTX.logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);
    CPU_CTX.logger_error = log_create("cpu_error.log", "CPU_ERROR", 1, LOG_LEVEL_ERROR);

    // Setear globals para extern en sub-módulos
    logger = CPU_CTX.logger;
    loggerError = CPU_CTX.logger_error;

    if (!CPU_CTX.logger || !CPU_CTX.logger_error) {
        return;
    }

    CPU_CTX.config = (t_cpu_config) cpu_cargar_config(path_config);
    cpu_imprimir_config(CPU_CTX.config);

    cpu_servidores_kernel_init(CPU_CTX.config.puerto_dispatch, CPU_CTX.config.puerto_interrupt);
    cpu_conexiones_memoria_init(CPU_CTX.config.ip_memoria, CPU_CTX.config.puerto_memoria);

    interrupciones_init();

    // Inicializar TLB
    bool tlb_lru = (strcmp(CPU_CTX.config.tlb_algoritmo, "LRU") == 0);
    tlb_init(CPU_CTX.config.tlb_cant_ent, tlb_lru);

    log_info(CPU_CTX.logger, "CPU inicializada");
}

void cpu_run(void) {
    log_info(CPU_CTX.logger, "CPU esperando peticiones Dispatch/Interrupt");
    while(1) {
        sleep(10); 
    }
}

void cpu_shutdown(void) {
    log_info(CPU_CTX.logger, "CPU apagando");

    cpu_conexiones_memoria_close();

    if (CPU_CTX.socket_dispatch >= 0) close(CPU_CTX.socket_dispatch);
    if (CPU_CTX.socket_interrupt >= 0) close(CPU_CTX.socket_interrupt);

    if (CPU_CTX.logger) log_destroy(CPU_CTX.logger);
    if (CPU_CTX.logger_error) log_destroy(CPU_CTX.logger_error);
}