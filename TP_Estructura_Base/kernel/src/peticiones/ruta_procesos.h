#ifndef RUTA_PROCESOS_H_
#define RUTA_PROCESOS_H_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * Construir nombre del proceso (con extensión .txt si no la tiene)
 * 
 * @param nombre_proceso Nombre del archivo (ej: "proceso1" o "proceso1.txt")
 * @return Nombre normalizado con extensión (ej: "proceso1.txt")
 * 
 * IMPORTANTE: El nombre devuelto debe ser liberado por el llamador
 * Este nombre es lo que se envía a Memoria en el campo "path" de t_mem_init_proceso
 */
char* construir_nombre_proceso(const char* nombre_proceso);

/**
 * Valida que el archivo de proceso existe desde la perspectiva de KERNEL
 * 
 * KERNEL se ejecuta desde /kernel, busca en ../memoria/procesos/
 * 
 * @param nombre_proceso Nombre del proceso (ej: "proceso1.txt")
 * @return true si el archivo existe, false si no
 */
bool validar_existe_proceso_kernel(const char* nombre_proceso);

#endif

