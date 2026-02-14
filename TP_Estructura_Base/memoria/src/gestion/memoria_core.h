#ifndef MEMORIA_CORE_H_
#define MEMORIA_CORE_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================
 * ESTRUCTURA INTERNA
 * ========================= */

typedef struct {
    uint32_t pid;
    char** instrucciones;     // array de strings
    uint32_t cantidad;        // cantidad de instrucciones
} t_proceso_memoria;

/* =========================
 * API CORE MEMORIA
 * ========================= */

// inicializa estructuras internas
int memoria_core_init(void);

// crea proceso y carga instrucciones desde archivo
bool memoria_crear_proceso(uint32_t pid, const char* path);

// elimina proceso y libera memoria
void memoria_destruir_proceso(uint32_t pid);

// devuelve instrucción asociada a PID y PC (memoria alocada, caller debe liberar)
// si PC inválido → devuelve strdup("EXIT")
char* memoria_fetch_instruccion(uint32_t pid, uint32_t pc);

#endif
