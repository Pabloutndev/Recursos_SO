#include "shared.h"
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

int existe_archivo(char* path)
{
    FILE* f = fopen(path, "r");
    if (f != NULL)
    {
        fclose(f);
        return 1;
    }
    return 0;
}

int existe_dir(char* path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

int string_arr_size(char** array)
{
    int i = 0;
    while(array && array[i] != NULL)
    {
        i++;
    }
    return i;
}

// ============================================================================
// HELPERS INTERNOS
// ============================================================================

static char* trim(char* str) {
    char* start = str;
    while (isspace((unsigned char)*start)) start++;
    
    if (*start == 0) return start;

    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';
    return start;
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
        char* trimmed = trim(line);

        if (strlen(trimmed) == 0) {
            continue;
        }

        if (*cantidad >= cap) {
            size_t new_cap = (cap == 0) ? 8 : cap * 2;
            char** temp = realloc(instrucciones, new_cap * sizeof(char*));
            if (!temp) {
                // Error de memoria: liberamos lo anterior para evitar leaks mayores
                liberar_instrucciones(instrucciones, *cantidad);
                free(line);
                fclose(f);
                *cantidad = 0;
                return NULL;
            }
            instrucciones = temp;
            cap = new_cap;
        }

        instrucciones[*cantidad] = strdup(trimmed);
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
