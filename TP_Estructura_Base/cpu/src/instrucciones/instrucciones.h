#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include <stdint.h>
#include <stdbool.h>
#include <registros/registros.h>
#include <contexto/contexto.h>

typedef enum {
    INST_SET,
    INST_SUM,
    INST_SUB,
    INST_JNZ,
    INST_IO,
    INST_EXIT,
    FIN_QUANTUM
} opcode_t;

typedef struct {
    opcode_t opcode;
    reg_id_t r1;
    reg_id_t r2;
    uint32_t inmediato;
} instruccion_t;

bool ejecutar_siguiente_instruccion(t_contexto_cpu* ctx);

#endif
