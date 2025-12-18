#ifndef MOD_KERNEL_H
#define MOD_KERNEL_H

#include <commons/log.h>
#include <config/kernel_config.h>

extern t_log* logger;
extern t_log* loggerError;
extern t_kernel_config KCONF;

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