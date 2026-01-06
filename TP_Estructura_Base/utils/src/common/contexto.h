
#ifndef SHARED_CONTEXTO_H_
#define SHARED_CONTEXTO_H_

#include <stdint.h>
#include <stdbool.h>


#include "registros.h"

// Contexto compartido

typedef struct {
    uint32_t pid;
    uint32_t quantum;
    registros_t registros;

    bool bloqueado; // State flags if needed
    bool finalizado;
    
    // Motivo de desalojo / IO
    uint32_t io_time;
    // int motivo_desalojo; // definido en op_code (packet header)
} contexto_t;

#endif
