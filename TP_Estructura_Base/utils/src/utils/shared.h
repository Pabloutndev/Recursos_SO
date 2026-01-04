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

#endif