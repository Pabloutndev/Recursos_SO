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
comando_t obtener_comando(const char* palabra);
int obtener_pid(char* token);
void comandos_destroy(void);

#endif /* CONSOLA_H */