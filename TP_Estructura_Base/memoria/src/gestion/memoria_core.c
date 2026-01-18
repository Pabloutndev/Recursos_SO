#include "memoria_core.h"

#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <commons/log.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern t_log* logger;
extern t_log* loggerError;

/* =========================
 * ESTRUCTURAS INTERNAS
 * ========================= */

static t_dictionary* procesos; // key = PID (string)
static char** leer_instrucciones(const char* path, uint32_t* cantidad);
static char* pid_key(uint32_t pid);

/* =========================
 * API PUBLICA
 * ========================= */

void memoria_core_init(void) {
    procesos = dictionary_create();
    log_info(logger, "MEMORIA: memoria_core inicializado");
}

bool memoria_crear_proceso(uint32_t pid, const char* path) {
    char* key = pid_key(pid);

    if (dictionary_has_key(procesos, key)) {
        log_warning(logger, "MEMORIA: Proceso %u ya existe", pid);
        free(key);
        return false;
    }

    t_proceso_memoria* proc = malloc(sizeof(t_proceso_memoria));
    proc->pid = pid;
    proc->instrucciones = leer_instrucciones(path, &proc->cantidad);

    if (!proc->instrucciones) {
        free(proc);
        free(key);
        return false;
    }

    dictionary_put(procesos, key, proc);

    log_info(logger, "MEMORIA: Proceso %u cargado (%u instrucciones)",
             pid, proc->cantidad);
    return true;
}

void memoria_destruir_proceso(uint32_t pid) {
    char* key = pid_key(pid);

    t_proceso_memoria* proc = dictionary_remove(procesos, key);
    free(key);

    if (!proc) return;

    for (uint32_t i = 0; i < proc->cantidad; i++) {
        free(proc->instrucciones[i]);
    }

    free(proc->instrucciones);
    free(proc);

    log_info(logger, "MEMORIA: Proceso %u destruido", pid);
}

const char* memoria_fetch_instruccion(uint32_t pid, uint32_t pc) {
    char* key = pid_key(pid);
    t_proceso_memoria* proc = dictionary_get(procesos, key);
    free(key);

    if (!proc) {
        log_error(loggerError, "MEMORIA: FETCH proceso inexistente PID=%u", pid);
        return "EXIT";
    }

    if (pc >= proc->cantidad) {
        log_warning(logger, "MEMORIA: FETCH fuera de rango PID=%u PC=%u", pid, pc);
        return "EXIT";
    }

    return proc->instrucciones[pc];
}


/* =========================
 * UTILS INTERNOS
 * ========================= */

static char** leer_instrucciones(const char* path, uint32_t* cantidad) {
    FILE* f = fopen(path, "r");
    if (!f) {
        log_error(loggerError, "MEMORIA: No se pudo abrir archivo %s", path);
        *cantidad = 0;
        return NULL;
    }

    char** instrucciones = NULL;
    size_t cap = 0;
    *cantidad = 0;

    char* line = NULL;
    size_t len = 0;

    while (getline(&line, &len, f) != -1) {
        string_trim(&line);

        if (*cantidad >= cap) {
            cap = cap == 0 ? 8 : cap * 2;
            instrucciones = realloc(instrucciones, cap * sizeof(char*));
        }

        instrucciones[*cantidad] = strdup(line);
        (*cantidad)++;
    }

    free(line);
    fclose(f);

    return instrucciones;
}

static char* pid_key(uint32_t pid) {
    return string_itoa(pid); // usar con cuidado, se libera al final
}