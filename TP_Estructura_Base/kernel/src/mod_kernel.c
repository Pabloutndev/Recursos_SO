// kernel.c
#include <mod_kernel.h>
#include <consola/consola.h>
#include <config/kernel_config.h>
#include <planificacion/planificacion.h>
#include <conexiones/cpu.h>
#include <conexiones/memoria.h>
#include <conexiones/io.h>
#include <peticiones/recursos.h>
#include <stdlib.h>

#define PUERTO "8005"

/// NOTE: VARIABLES GLOBALES DEL MODULO
t_log* logger;
t_log* loggerError;
t_kernel_config KCONF;
int socket_dispatch = -1;
int socket_interrupt = -1;
int socket_memoria = -1;

void conexiones_init()
{
    // Conectar a Memoria
    conectar_memoria(KCONF.ip_memoria, KCONF.puerto_memoria);

    // Conectar a CPU
    conectar_cpu(KCONF.ip_cpu, KCONF.puerto_cpu_dispatch, KCONF.puerto_cpu_interrupt);

    // TODO: Conectar a fs (SI APLICA)
    // socket_fs = ...
    
    // Iniciar Server para IO (SI APLICA)
    // server_kernel_init(...);
}

void terminar(int sig) {
    log_info(logger,"Recibida señal %d. Terminando kernel...", sig);
    kernel_shutdown();
    exit(EXIT_SUCCESS);
}

int kernel_init(const char* config_path) 
{
    // iniciar logs (usá tus utils)
    logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
    loggerError = log_create("kernel_error.log", "KERNEL_ERR", 1, LOG_LEVEL_ERROR);

    // cargar config (usá tu utils)
    // ejemplo de asignación:
    KCONF = kernel_cargar_config(config_path);
    
    kernel_imprimir_config(KCONF);

    // Inicializar conexiones (CPU, Memoria)
    conexiones_init();
    
    // Iniciar Server para IO
    server_io_init(PUERTO); 
    
    // Iniciar Recursos
    recursos_init(&KCONF);
    // Si no, hardcodeo o busco en config
    // Revisar struct t_kernel_config si es necesario.
    // Por ahora uso un puerto generic si no esta en config, o KCONF.puerto_escucha
    // KCONF no parece tener puerto_escucha en el view anterior, solo ip/puertos de otros.
    // Asumire que existe o usan PUERTO define.
    server_io_init(PUERTO); 

    planificacion_init();

    iniciar_consola();
    
    return EXIT_SUCCESS;
}

void kernel_shutdown(void) {
    log_info(logger,"Shutdown kernel...");
    planificacion_destroy();
    recursos_destroy();
    if (logger) log_destroy(logger);
    if (loggerError) log_destroy(loggerError);
}