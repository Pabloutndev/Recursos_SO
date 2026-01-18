#include <cpu.h>
#include <config/cpu_config.h>
#include <loggers/logger.h>
#include <registros/registros.h>
#include <interrupciones/interrupciones.h>
#include <adaptadores/contexto_cpu_adapter.h>
#include <protocolo/mensajes.h>

t_log* logger;
t_log* loggerError;
t_cpu_config CPU_CONF;
t_contexto_cpu cpu_estado;
t_motivo_desalojo motivo_desalojo;
int socket_dispatch = -1;
int socket_interrupt = -1;

void cpu_init(const char* path_config)
{
    logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);
    loggerError = log_create("cpu_error.log", "CPU_ERROR", 1, LOG_LEVEL_ERROR);

    CPU_CONF = (t_cpu_config) cpu_cargar_config(path_config);
    
    cpu_imprimir_config(CPU_CONF);

    cpu_servidores_kernel_init(CPU_CONF.puerto_dispatch, CPU_CONF.puerto_interrupt);

    cpu_conexiones_memoria_init(CPU_CONF.ip_memoria, CPU_CONF.puerto_memoria);

    interrupciones_init();

    log_info(logger, "CPU inicializada correctamente");
}

void cpu_run(void) {
    while (true) {
        t_contexto_cpu ctx;
        
        if (!recibir_contexto_kernel(&ctx)){
            motivo_desalojo = MOTIVO_DESALOJO;
            break;
        }

        ciclo_instruccion_ejecutar(&ctx);
        
        enviar_contexto_kernel(&ctx, motivo_desalojo);
    }
}

void cpu_shutdown(void) {
    //conexiones_kernel_close();
    //conexiones_memoria_close();
    log_destroy(logger);
}