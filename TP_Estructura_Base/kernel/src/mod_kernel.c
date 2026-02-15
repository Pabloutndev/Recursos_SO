// mod_kernel.c
#include <mod_kernel.h>
#include <consola/consola.h>
#include <config/kernel_config.h>
#include <planificacion/planificacion.h>
#include <conexiones/cpu.h>
#include <conexiones/memoria.h>
#include <conexiones/io.h>
#include <peticiones/recursos.h>
#include <stdlib.h>
#include <unistd.h>
#include <server/server.h>

#define PUERTO_IO_DEFAULT "8005"

/// Contexto Global del Modulo
t_kernel_context KERNEL_CTX;

// Globals para extern en sub-modulos (apuntan a KERNEL_CTX)
t_log* logger = NULL;
t_log* loggerError = NULL;
int socket_dispatch = -1;
int socket_interrupt = -1;
int socket_memoria = -1;

void conexiones_init()
{
    // Conectar a Memoria
    conectar_memoria(KERNEL_CTX.config.ip_memoria, KERNEL_CTX.config.puerto_memoria);

    // Conectar a CPU
    conectar_cpu(KERNEL_CTX.config.ip_cpu, KERNEL_CTX.config.puerto_cpu_dispatch, KERNEL_CTX.config.puerto_cpu_interrupt);
}

void terminar(int sig) {
    log_info(KERNEL_CTX.logger, "Kernel: senal %d recibida, terminando", sig);
    kernel_shutdown();
    exit(EXIT_SUCCESS);
}

int kernel_init(const char* config_path) 
{
    // Iniciar Logs
    KERNEL_CTX.logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
    KERNEL_CTX.logger_error = log_create("kernel_error.log", "KERNEL_ERR", 1, LOG_LEVEL_ERROR);

    if (!KERNEL_CTX.logger || !KERNEL_CTX.logger_error) {
        return EXIT_FAILURE;
    }

    // Setear globals para extern en sub-modulos
    logger = KERNEL_CTX.logger;
    loggerError = KERNEL_CTX.logger_error;

    // Cargar Configuración
    KERNEL_CTX.config = kernel_cargar_config(config_path);
    kernel_imprimir_config(KERNEL_CTX.config);

    // Inicializar conexiones (CPU, Memoria)
    conexiones_init();
    
    // Iniciar Server para IO
    server_io_init(PUERTO_IO_DEFAULT); 
    
    // Lanzar escucha del server IO
    kernel_server_io_listen(PUERTO_IO_DEFAULT);
    
    // Iniciar Recursos
    recursos_init(&KERNEL_CTX.config);

    planificacion_init();

    // Iniciar planificadores (largo y corto plazo)
    planificacion_start();

    // Lanzar server para Consola remota
    kernel_server_consola_listen(KERNEL_CTX.config.puerto_consola);
    log_info(KERNEL_CTX.logger, "Kernel: listo, consola en puerto %s", KERNEL_CTX.config.puerto_consola);

    // Consola local como fallback (bloquea el hilo main)
    iniciar_consola();

    return EXIT_SUCCESS;
}

void kernel_shutdown(void) {
    log_info(KERNEL_CTX.logger, "Kernel: apagando");

    // 1. Destruir planificacion (join hilos, destruir colas/mutex/sems)
    planificacion_destroy();

    // 2. Liberar recursos (semaforos de recursos, colas de bloqueados por recurso)
    recursos_destroy();

    // 3. Cerrar sockets de conexion a CPU y Memoria
    if (KERNEL_CTX.socket_dispatch >= 0) {
        close(KERNEL_CTX.socket_dispatch);
        KERNEL_CTX.socket_dispatch = -1;
    }
    if (KERNEL_CTX.socket_interrupt >= 0) {
        close(KERNEL_CTX.socket_interrupt);
        KERNEL_CTX.socket_interrupt = -1;
    }
    if (KERNEL_CTX.socket_memoria >= 0) {
        close(KERNEL_CTX.socket_memoria);
        KERNEL_CTX.socket_memoria = -1;
    }

    log_info(KERNEL_CTX.logger, "Kernel: apagado correctamente");

    // 4. Destruir loggers (al final, despues de loguear)
    if (KERNEL_CTX.logger) {
        log_destroy(KERNEL_CTX.logger);
        KERNEL_CTX.logger = NULL;
    }
    if (KERNEL_CTX.logger_error) {
        log_destroy(KERNEL_CTX.logger_error);
        KERNEL_CTX.logger_error = NULL;
    }
}
