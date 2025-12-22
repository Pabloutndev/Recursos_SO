#include <ciclo_instruccion/decode.h>
#include <instrucciones/instrucciones.h>
#include <commons/string.h>
#include <stdlib.h>

static reg_id_t parse_registro(char* r)
{
    if (!strcmp(r,"AX")) return REG_AX;
    if (!strcmp(r,"BX")) return REG_BX;
    if (!strcmp(r,"CX")) return REG_CX;
    if (!strcmp(r,"DX")) return REG_DX;
    if (!strcmp(r,"EAX")) return REG_EAX;
    if (!strcmp(r,"EBX")) return REG_EBX;
    if (!strcmp(r,"ECX")) return REG_ECX;
    if (!strcmp(r,"EDX")) return REG_EDX;
    if (!strcmp(r,"SI")) return REG_SI;
    if (!strcmp(r,"DI")) return REG_DI;
    return REG_PC;
}

instruccion_t decoder_parsear(char* linea)
{
    char** t = string_split(linea, " ");
    instruccion_t inst = {0};

    if (!strcmp(t[0],"SET")) {
        inst.opcode = INST_SET;
        inst.r1 = parse_registro(t[1]);
        inst.inmediato = atoi(t[2]);
    }
    else if (!strcmp(t[0],"SUM")) {
        inst.opcode = INST_SUM;
        inst.r1 = parse_registro(t[1]);
        inst.r2 = parse_registro(t[2]);
    }
    else if (!strcmp(t[0],"SUB")) {
        inst.opcode = INST_SUB;
        inst.r1 = parse_registro(t[1]);
        inst.r2 = parse_registro(t[2]);
    }
    else if (!strcmp(t[0],"JNZ")) {
        inst.opcode = INST_JNZ;
        inst.inmediato = atoi(t[1]);
    }
    else if (!strcmp(t[0],"IO")) {
        inst.opcode = INST_IO;
        inst.inmediato = atoi(t[1]);
    }
    else {
        inst.opcode = INST_EXIT;
    }

    string_array_destroy(t);
    return inst;
}
