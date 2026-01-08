
#ifndef SHARED_REGISTROS_H
#define SHARED_REGISTROS_H

#include <stdint.h>

/// TODO: REVISAR QUIZÁS NO SE USA....
/// se deberia colocar en modulo cpu...
/// red_id_t , registros_t esta bien acá
typedef enum {
    REG_AX, REG_BX, REG_CX, REG_DX,
    REG_EAX, REG_EBX, REG_ECX, REG_EDX,
    REG_SI, REG_DI,
    REG_PC
} reg_id_t;

typedef struct {
    uint8_t AX, BX, CX, DX;
    uint32_t EAX, EBX, ECX, EDX;
    uint32_t SI, DI;
    uint32_t PC;
} registros_t;

#endif
