
#ifndef SHARED_CONTEXTO_H_
#define SHARED_CONTEXTO_H_

#include <stdint.h>
#include <stdbool.h>
#include "registros.h"

typedef struct {
    uint32_t pid;
    uint32_t pc;
    uint32_t quantum;
    registros_t registros;
} t_contexto_cpu;

#endif
