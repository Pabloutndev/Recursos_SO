#ifndef MOD_KERNEL_H
#define MOD_KERNEL_H

#include <commons/log.h>
#include <loggers/logger.h>
#include <config/kernel_config.h>

#include <conexiones/cpu.h>
#include <conexiones/memoria.h>

typedef struct {
    t_log* logger;
    t_log* logger_error;
    t_kernel_config config;
    int socket_dispatch;
    int socket_interrupt;
    int socket_memoria;
} t_kernel_context;

extern t_kernel_context KERNEL_CTX;

///TODO: utiliza utils/conexiones y conexiones/
// server_kernel
// cliente fs mem cpu...

/**
 * @brief Apaga el modulo kernel
 * @param void sin parametros
 * @return void no hay retorno
*/
void terminar(int sig);

/**
 * @brief Inicializa la estructura config del kernel
 * @param char* ruta del archivo config del kernel
 * @return int 
*/
int kernel_init(const char* config_path);

/**
 * @brief Apaga el modulo kernel
 * @param void sin parametros
 * @return void no hay retorno
*/
void kernel_shutdown(void);

#endif /*MOD_KERNEL_H*/