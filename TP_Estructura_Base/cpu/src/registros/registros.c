#include "registros.h"

uint32_t registros_leer(registros_t* r, reg_id_t reg)
{
    switch (reg) {
        case REG_AX:  return r->AX;
        case REG_BX:  return r->BX;
        case REG_CX:  return r->CX;
        case REG_DX:  return r->DX;
        case REG_EAX: return r->EAX;
        case REG_EBX: return r->EBX;
        case REG_ECX: return r->ECX;
        case REG_EDX: return r->EDX;
        case REG_SI:  return r->SI;
        case REG_DI:  return r->DI;
        case REG_PC:  return r->PC;
        default:      return 0;
    }
}

void registros_escribir(registros_t* r, reg_id_t reg, uint32_t valor)
{
    switch (reg) {
        case REG_AX:  r->AX  = (uint8_t)valor; break;
        case REG_BX:  r->BX  = (uint8_t)valor; break;
        case REG_CX:  r->CX  = (uint8_t)valor; break;
        case REG_DX:  r->DX  = (uint8_t)valor; break;
        case REG_EAX: r->EAX = valor; break;
        case REG_EBX: r->EBX = valor; break;
        case REG_ECX: r->ECX = valor; break;
        case REG_EDX: r->EDX = valor; break;
        case REG_SI:  r->SI  = valor; break;
        case REG_DI:  r->DI  = valor; break;
        case REG_PC:  r->PC  = valor; break;
    }
}

void registros_reset(registros_t* r)
{
    *r = (registros_t){0};
}
