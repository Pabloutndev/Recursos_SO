#include <cpu.h>
#include <config/cpu_config.h>
#include <loggers/logger.h>
#include <registros/registros.h>
#include <interrupciones/interrupciones.h>

t_log* logger;
t_log* loggerError;
t_cpu_config CPU_CONF;

void cpu_init(const char* path_config)
{
    logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);
    loggerError = log_create("cpu_error.log", "CPU_ERROR", 1, LOG_LEVEL_ERROR);

    CPU_CONF = (t_cpu_config) cpu_cargar_config(path_config);

    cpu_imprimir_config(CPU_CONF);

    cpu_servidores_kernel_init(CPU_CONF.puerto_dispatch, CPU_CONF.puerto_interrupt);

    interrupciones_init();
    //registros_init();

    log_info(logger, "CPU inicializada correctamente");
}

void cpu_run(void) {
    while (true) {
        contexto_t ctx;

        ctx.pid = 4;
        ctx.quantum = 2000;

        /* TODO: 
        if (!kernel_recibir_contexto(&ctx)) {
            log_info(logger, "Esperando contexto...");
            continue;
        }*/

        ciclo_instruccion_ejecutar(&ctx);

        //kernel_enviar_contexto(&ctx);
    }
}

void cpu_shutdown(void) {
    //conexiones_kernel_close();
    //conexiones_memoria_close();
    log_destroy(logger);
}