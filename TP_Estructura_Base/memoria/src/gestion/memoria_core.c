#include "memoria_core.h"

#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <commons/log.h>
#include <common/shared.h>
#include <swap/swap.h>
#include <gestion/paginas.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern t_log* logger;
extern t_log* loggerError;

/* =========================
 * CONFIGURACION
 * ========================= */

// MEMORIA se ejecuta desde /memoria, los procesos están en ./procesos/
#define RUTA_BASE_PROCESOS_MEMORIA "procesos/"

/* =========================
 * ESTRUCTURAS INTERNAS
 * ========================= */

static t_dictionary* procesos; // key = PID (string)
static char* pid_key(uint32_t pid);

// HELPER: Construye ruta desde perspectiva de MEMORIA
static char* construir_ruta_proceso_memoria(const char* nombre_proceso) {
    char* ruta = malloc(512);
    if (!ruta) return NULL;
    snprintf(ruta, 512, "%s%s", RUTA_BASE_PROCESOS_MEMORIA, nombre_proceso);
    return ruta;
}

/* =========================
 * API PUBLICA
 * ========================= */

int memoria_core_init(void) {
    procesos = dictionary_create();
    if (!procesos) return -1;
    log_info(logger, "MEMORIA: memoria_core inicializado");
    return 0;
}

bool memoria_crear_proceso(uint32_t pid, const char* path) {
    char* key = pid_key(pid);

    if (dictionary_has_key(procesos, key)) {
        log_warning(logger, "MEMORIA: Proceso %u ya existe", pid);
        free(key);
        return false;
    }

    // 'path' recibido desde Kernel es en realidad el NOMBRE del proceso
    // Memoria construye su propia ruta relativa
    char* ruta_memoria = construir_ruta_proceso_memoria(path);
    if (!ruta_memoria) {
        log_error(loggerError, "MEMORIA: No se pudo construir ruta para proceso");
        free(key);
        return false;
    }

    log_info(logger, "MEMORIA: Leyendo instrucciones - Nombre: %s, Ruta local: %s, PID: %u", path, ruta_memoria, pid);

    t_proceso_memoria* proc = malloc(sizeof(t_proceso_memoria));
    proc->pid = pid;
    proc->instrucciones = leer_instrucciones(ruta_memoria, &proc->cantidad);
    free(ruta_memoria);

    if (!proc->instrucciones) {
        log_error(loggerError, "MEMORIA: No se pudieron leer instrucciones de %s", path);
        free(proc);
        free(key);
        return false;
    }

    dictionary_put(procesos, key, proc);

    // Crear paginacion con el tamanio correcto: cada instruccion ocupa 64 bytes
    uint32_t tamanio = proc->cantidad * 64;
    if (!paginacion_crear_proceso(pid, tamanio)) {
        log_error(loggerError, "MEMORIA: Fallo creando paginacion para PID %u (tamanio=%u)", pid, tamanio);
        // Rollback: remover del diccionario
        dictionary_remove(procesos, key);
        for (uint32_t i = 0; i < proc->cantidad; i++) free(proc->instrucciones[i]);
        free(proc->instrucciones);
        free(proc);
        free(key);
        return false;
    }

    // Cargar instrucciones en paginas
    log_info(logger, "MEMORIA: Cargando %d instrucciones en RAM para PID %u", proc->cantidad, pid);
    for (int i = 0; i < proc->cantidad; i++) {
        uint32_t dir_logica = i * 64;
        paginacion_escribir(pid, dir_logica, proc->instrucciones[i], strlen(proc->instrucciones[i]) + 1);
    }

    free(key);
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

    swap_borrar_proceso(pid);

    log_info(logger, "MEMORIA: Proceso %u destruido", pid);
}

char* memoria_fetch_instruccion(uint32_t pid, uint32_t pc) {
    /* Mantenemos el control administrativo por ahora */
    char* key = pid_key(pid);
    t_proceso_memoria* proc = dictionary_get(procesos, key);
    free(key);

    if (!proc) {
        log_error(loggerError, "MEMORIA: FETCH proceso inexistente PID=%u", pid);
        return strdup("EXIT");
    }

    /* UNIFICACIÓN: Usar el sistema de paginación para leer la instrucción */
    log_info(logger, "MEMORIA: Usando paginación para fetch PID=%u PC=%u", pid, pc);
    char* instruccion_leida = paginacion_leer_instruccion(pid, pc);

    if (instruccion_leida != NULL) {
        return instruccion_leida;
    }

    /* Fallback: leer del array de instrucciones en memoria */
    if (pc < proc->cantidad) {
        return strdup(proc->instrucciones[pc]);
    }

    return strdup("EXIT");
}


/* =========================
 * UTILS INTERNOS
 * ========================= */

static char* pid_key(uint32_t pid) {
    return string_itoa(pid); // usar con cuidado, se libera al final
}