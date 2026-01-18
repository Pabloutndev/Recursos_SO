#include "shared.h"
#include <string.h>

int existe_archivo(char* path)
{
	int ret = 0;
	FILE* f = fopen(path,"r");
	if (f != NULL)
	{
		ret = 1;
		fclose(f);
	}
	return ret;
}
/*
int existe_dir(char* path)
{
	DIR* dir = opendir(path);
	if (dir) {
		closedir(dir);
		return 1;
	} else if (ENOENT == errno) {
		return 0;
	} else {
		return -1;
	}
}
*/
int string_arr_size(char** a)
{
    int i = 0;
    while(a[i] != NULL)
    {
        i++;
    }
    return i;
}

// ============================================================================
// LECTURA DE INSTRUCCIONES
// ============================================================================

char** leer_instrucciones(const char* path, uint32_t* cantidad) {
    FILE* f = fopen(path, "r");
    if (!f) {
        *cantidad = 0;
        return NULL;
    }

    char** instrucciones = NULL;
    size_t cap = 0;
    *cantidad = 0;

    char* line = NULL;
    size_t len = 0;

    while (getline(&line, &len, f) != -1) {
        // Trim whitespace
        char* start = line;
        while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') {
            start++;
        }
        char* end = start + strlen(start) - 1;
        while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
            end--;
        }
        *(end + 1) = '\0';

        // Skip empty lines
        if (strlen(start) == 0) {
            continue;
        }

        if (*cantidad >= cap) {
            cap = cap == 0 ? 8 : cap * 2;
            instrucciones = realloc(instrucciones, cap * sizeof(char*));
        }

        instrucciones[*cantidad] = malloc(strlen(start) + 1);
        strcpy(instrucciones[*cantidad], start);
        (*cantidad)++;
    }

    free(line);
    fclose(f);

    return instrucciones;
}

void liberar_instrucciones(char** instrucciones, uint32_t cantidad) {
    if (!instrucciones) return;
    
    for (uint32_t i = 0; i < cantidad; i++) {
        free(instrucciones[i]);
    }
    free(instrucciones);
}