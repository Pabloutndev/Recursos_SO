#ifndef CONTEXTO_H
#define CONTEXTO_H

#include <stdint.h>
#include <stdbool.h>
#include <registros/registros.h>

typedef struct {
    uint32_t pid;
    uint32_t quantum;
    registros_t registros;

    bool bloqueado;
    bool finalizado;
    uint32_t io_time;
} contexto_t;

void contexto_reset(contexto_t* ctx);

#endif
