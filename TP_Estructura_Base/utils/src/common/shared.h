#ifndef _UTILS_SHARED_H_
#define _UTILS_SHARED_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Defines Generales */
#define PAGE_FAULT_ERROR -1

/* Funciones Compartidas */
int existe_archivo(char* path);
int existe_dir(char* path);
int string_arr_size(char** a);

/**
 * Lee las instrucciones de un archivo de proceso
 * 
 * @param path Ruta al archivo de proceso (ej: "../memoria/procesos/process1.txt")
 * @param cantidad Puntero donde se guardará la cantidad de instrucciones
 * @return Array de strings (instrucciones), o NULL si falla
 * 
 * IMPORTANTE: El llamador debe liberar el array y cada string
 */
char** leer_instrucciones(const char* path, uint32_t* cantidad);

/**
 * Libera la memoria de un array de instrucciones
 * 
 * @param instrucciones Array de strings
 * @param cantidad Cantidad de strings en el array
 */
void liberar_instrucciones(char** instrucciones, uint32_t cantidad);

#endif