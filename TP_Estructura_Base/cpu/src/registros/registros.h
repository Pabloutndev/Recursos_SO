#ifndef REGISTROS_H
#define REGISTROS_H

#include <stdint.h>

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

uint32_t registros_leer(registros_t* r, reg_id_t reg);
void registros_escribir(registros_t* r, reg_id_t reg, uint32_t valor);
void registros_reset(registros_t* r);

#endif