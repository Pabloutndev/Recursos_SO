#ifndef CONSOLA_H
#define CONSOLA_H

#include <stdbool.h>

typedef enum {
    CMD_RUN,
    CMD_KILL,
    CMD_PS,
    CMD_ALGORITMO,
    CMD_START,
    CMD_PAUSE,
    CMD_DESALOJAR,
    CMD_EXIT,
    CMD_UNKNOWN
} comando_t;

void init_comandos(void);
void mensaje_inicial(void);
bool procesar_linea(char* linea);
void iniciar_consola(void);
static comando_t obtener_comando(const char* palabra);
static int obtener_pid(char* token);

#endif /* CONSOLA_H */