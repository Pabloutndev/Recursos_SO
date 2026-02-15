#ifndef CONSOLA_HELPERS_H
#define CONSOLA_HELPERS_H

#include <stdint.h>
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
    CMD_HELP,
    CMD_UNKNOWN
} comando_t;

void init_comandos(void);
void destroy_comandos(void);
comando_t obtener_comando(const char* palabra);
int obtener_pid(char* token);
void mensaje_inicial(void);

#endif
