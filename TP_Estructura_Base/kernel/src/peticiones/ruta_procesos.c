#include "ruta_procesos.h"
#include <stdio.h>

// KERNEL: Cuando kernel se ejecuta desde /kernel, los procesos están en ../memoria/procesos/
// Esta función VALIDA que el archivo existe localmente desde la perspectiva de kernel
// Pero envía solo el NOMBRE a Memoria (no la ruta kernel-relativa)
#define RUTA_BASE_PROCESOS_KERNEL "../memoria/procesos/"
#define EXTENSION_PROCESO ".txt"

char* construir_nombre_proceso(const char* nombre_proceso) {
    if (!nombre_proceso || strlen(nombre_proceso) == 0) {
        return NULL;
    }

    // Alocar espacio para nombre + extensión + null
    char* nombre = malloc(256);
    if (!nombre) return NULL;

    // Verificar si ya tiene extensión
    const char* ext = strrchr(nombre_proceso, '.');
    
    if (ext && strcmp(ext, EXTENSION_PROCESO) == 0) {
        // Ya tiene .txt, mantener como está
        snprintf(nombre, 256, "%s", nombre_proceso);
    } else {
        // Agregar .txt
        snprintf(nombre, 256, "%s%s", nombre_proceso, EXTENSION_PROCESO);
    }

    return nombre;
}

// Valida que el archivo existe desde la perspectiva de KERNEL
// Retorna true si existe, false si no
// Esta es una validación local, no la ruta que se envía a Memoria
bool validar_existe_proceso_kernel(const char* nombre_proceso) {
    if (!nombre_proceso || strlen(nombre_proceso) == 0) {
        return false;
    }

    // Construir ruta desde perspectiva de kernel
    char ruta_kernel[512];
    snprintf(ruta_kernel, sizeof(ruta_kernel), "%s%s", RUTA_BASE_PROCESOS_KERNEL, nombre_proceso);

    // Intentar abrir archivo
    FILE* f = fopen(ruta_kernel, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

