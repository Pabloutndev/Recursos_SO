#include <ciclo_instruccion/decode.h>
#include <instrucciones/instrucciones.h>
#include <commons/string.h>
#include <commons/log.h>
#include <model/model.h>
#include <cpu.h>
#include <stdlib.h>
#include <string.h>

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
    return REG_AX;
}

instruccion_t decoder_parsear(char* linea)
{
    char** t = string_split(linea, " ");
    instruccion_t inst = {0};

    // Contar tokens
    int cant_tokens = 0;
    for (int i = 0; t[i] != NULL; i++) cant_tokens++;

    if (cant_tokens < 1) {
        log_error(CPU_CTX.logger, "DECODE: linea vacia, no hay tokens");
        string_array_destroy(t);
        inst.opcode = INST_EXIT;
        return inst;
    }

    if (!strcmp(t[0],"SET")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: SET requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_SET;
        inst.r1 = parse_registro(t[1]);
        inst.inmediato = atoi(t[2]);
    }
    else if (!strcmp(t[0],"SUM")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: SUM requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_SUM;
        inst.r1 = parse_registro(t[1]);
        inst.r2 = parse_registro(t[2]);
    }
    else if (!strcmp(t[0],"SUB")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: SUB requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_SUB;
        inst.r1 = parse_registro(t[1]);
        inst.r2 = parse_registro(t[2]);
    }
    else if (!strcmp(t[0],"JNZ")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: JNZ requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_JNZ;
        // JNZ REG INST -> Salta a INST si REG != 0
        inst.r1 = parse_registro(t[1]);
        inst.inmediato = atoi(t[2]);
    }
    else if (!strcmp(t[0],"IO_GEN_SLEEP")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: IO_GEN_SLEEP requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_GEN_SLEEP;
        strcpy(inst.parametros, t[1]);
        inst.inmediato = atoi(t[2]);
    }
    else if (!strcmp(t[0],"IO_STDIN_READ")) {
        if (cant_tokens < 4) { log_error(CPU_CTX.logger, "DECODE: IO_STDIN_READ requiere 4 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_STDIN_READ;
        strcpy(inst.parametros, t[1]);
        inst.r1 = parse_registro(t[2]); // Dir
        inst.r2 = parse_registro(t[3]); // Size
    }
    else if (!strcmp(t[0],"IO_STDOUT_WRITE")) {
        if (cant_tokens < 4) { log_error(CPU_CTX.logger, "DECODE: IO_STDOUT_WRITE requiere 4 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_STDOUT_WRITE;
        strcpy(inst.parametros, t[1]);
        inst.r1 = parse_registro(t[2]);
        inst.r2 = parse_registro(t[3]);
    }
    else if (!strcmp(t[0],"IO_FS_CREATE")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: IO_FS_CREATE requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_FS_CREATE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");     // Interfaz + Archivo
        strcat(inst.parametros, t[2]);
    }
    else if (!strcmp(t[0],"IO_FS_DELETE")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: IO_FS_DELETE requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_FS_DELETE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
    }
    else if (!strcmp(t[0],"IO_FS_TRUNCATE")) {
        if (cant_tokens < 4) { log_error(CPU_CTX.logger, "DECODE: IO_FS_TRUNCATE requiere 4 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_FS_TRUNCATE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
        inst.r1 = parse_registro(t[3]); // Reg Tam
    }
    else if (!strcmp(t[0],"IO_FS_WRITE")) {
        if (cant_tokens < 6) { log_error(CPU_CTX.logger, "DECODE: IO_FS_WRITE requiere 6 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_FS_WRITE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
        inst.r1 = parse_registro(t[3]); // Reg Dir
        inst.r2 = parse_registro(t[4]); // Reg Size
        inst.inmediato = atoi(t[5]);    // Ptr Archivo
    }
    else if (!strcmp(t[0],"IO_FS_READ")) {
        if (cant_tokens < 6) { log_error(CPU_CTX.logger, "DECODE: IO_FS_READ requiere 6 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_IO_FS_READ;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
        inst.r1 = parse_registro(t[3]);
        inst.r2 = parse_registro(t[4]);
        inst.inmediato = atoi(t[5]);
    }
    else if (!strcmp(t[0],"WAIT")) {
        if (cant_tokens < 2) { log_error(CPU_CTX.logger, "DECODE: WAIT requiere 2 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_WAIT;
        strcpy(inst.parametros, t[1]);
    }
    else if (!strcmp(t[0],"SIGNAL")) {
        if (cant_tokens < 2) { log_error(CPU_CTX.logger, "DECODE: SIGNAL requiere 2 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_SIGNAL;
        strcpy(inst.parametros, t[1]);
    }
    else if (!strcmp(t[0],"MOV_IN")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: MOV_IN requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_MOV_IN; // MOV_IN REG DIR_LOGICA(REG)
        inst.r1 = parse_registro(t[1]);
        inst.r2 = parse_registro(t[2]);
    }
    else if (!strcmp(t[0],"MOV_OUT")) {
        if (cant_tokens < 3) { log_error(CPU_CTX.logger, "DECODE: MOV_OUT requiere 3 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_MOV_OUT;
        inst.r1 = parse_registro(t[1]); // DIR
        inst.r2 = parse_registro(t[2]); // REG
    }
    else if (!strcmp(t[0],"RESIZE")) {
        if (cant_tokens < 2) { log_error(CPU_CTX.logger, "DECODE: RESIZE requiere 2 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_RESIZE;
        inst.inmediato = atoi(t[1]);
    }
    else if (!strcmp(t[0],"COPY_STRING")) {
        if (cant_tokens < 2) { log_error(CPU_CTX.logger, "DECODE: COPY_STRING requiere 2 tokens, recibidos %d", cant_tokens); inst.opcode = INST_EXIT; string_array_destroy(t); return inst; }
        inst.opcode = INST_COPY_STRING;
        inst.inmediato = atoi(t[1]);
    }
    else {
        inst.opcode = INST_EXIT;
    }

    string_array_destroy(t);
    return inst;
}
