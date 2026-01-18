#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include <stdint.h>
#include <stdbool.h>
#include <registros/registros.h>
#include <model/model.h>

typedef enum {
    INST_SET,
    INST_SUM,
    INST_SUB,
    INST_JNZ,
    INST_IO,
    INST_IO_GEN_SLEEP,
    INST_IO_STDIN_READ,
    INST_IO_STDOUT_WRITE,
    INST_IO_FS_CREATE,
    INST_IO_FS_DELETE,
    INST_IO_FS_TRUNCATE,
    INST_IO_FS_WRITE,
    INST_IO_FS_READ,
    INST_MOV_IN,
    INST_MOV_OUT,
    INST_RESIZE,
    INST_COPY_STRING,
    INST_WAIT,
    INST_SIGNAL,
    INST_EXIT,
    FIN_QUANTUM
} opcode_t;

typedef struct {
    opcode_t opcode;
    reg_id_t r1;
    reg_id_t r2;
    uint32_t inmediato;
    char parametros[256];
} instruccion_t;

bool ejecutar_siguiente_instruccion(t_contexto_cpu* ctx);

#endif
