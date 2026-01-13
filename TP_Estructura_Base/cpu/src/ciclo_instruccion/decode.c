#include <ciclo_instruccion/decode.h>
#include <instrucciones/instrucciones.h>
#include <commons/string.h>
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
        // JNZ REY 10 -> Salta a 10 si REY != 0.
        // t[1] registro, t[2] inmediato?? Or JNZ 10 check status flag?
        // Revisar operaciones.c implementation. It checks REG_PC?
        // Standard: JNZ REG INST
        inst.r1 = parse_registro(t[1]);
        inst.inmediato = atoi(t[2]);
    }
    else if (!strcmp(t[0],"IO_GEN_SLEEP")) {
        inst.opcode = INST_IO_GEN_SLEEP;
        strcpy(inst.parametros, t[1]);
        inst.inmediato = atoi(t[2]);
    }
    else if (!strcmp(t[0],"IO_STDIN_READ")) {
        inst.opcode = INST_IO_STDIN_READ;
        strcpy(inst.parametros, t[1]);
        inst.r1 = parse_registro(t[2]); // Dir
        inst.r2 = parse_registro(t[3]); // Size
    }
    else if (!strcmp(t[0],"IO_STDOUT_WRITE")) {
        inst.opcode = INST_IO_STDOUT_WRITE;
        strcpy(inst.parametros, t[1]);
        inst.r1 = parse_registro(t[2]); 
        inst.r2 = parse_registro(t[3]);
    }
    else if (!strcmp(t[0],"IO_FS_CREATE")) {
        inst.opcode = INST_IO_FS_CREATE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");     // Interfaz + Archivo
        strcat(inst.parametros, t[2]);
    }
    else if (!strcmp(t[0],"IO_FS_DELETE")) {
        inst.opcode = INST_IO_FS_DELETE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
    }
    else if (!strcmp(t[0],"IO_FS_TRUNCATE")) {
        inst.opcode = INST_IO_FS_TRUNCATE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
        inst.r1 = parse_registro(t[3]); // Reg Tam
    }
    else if (!strcmp(t[0],"IO_FS_WRITE")) {
        inst.opcode = INST_IO_FS_WRITE;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
        inst.r1 = parse_registro(t[3]); // Reg Dir
        inst.r2 = parse_registro(t[4]); // Reg Size
        inst.inmediato = atoi(t[5]);    // Ptr Archivo
    }
    else if (!strcmp(t[0],"IO_FS_READ")) {
        inst.opcode = INST_IO_FS_READ;
        strcpy(inst.parametros, t[1]);
        strcat(inst.parametros, " ");
        strcat(inst.parametros, t[2]);
        inst.r1 = parse_registro(t[3]); 
        inst.r2 = parse_registro(t[4]);
        inst.inmediato = atoi(t[5]);
    }
    else if (!strcmp(t[0],"WAIT")) {
        inst.opcode = INST_WAIT;
        strcpy(inst.parametros, t[1]);
    }
    else if (!strcmp(t[0],"SIGNAL")) {
        inst.opcode = INST_SIGNAL;
        strcpy(inst.parametros, t[1]);
    }
    else if (!strcmp(t[0],"MOV_IN")) {
        inst.opcode = INST_MOV_IN; // MOV_IN REG DIR_LOGICA(REG)
        inst.r1 = parse_registro(t[1]);
        inst.r2 = parse_registro(t[2]);
    }
    else if (!strcmp(t[0],"MOV_OUT")) {
        inst.opcode = INST_MOV_OUT;
        inst.r1 = parse_registro(t[1]); // DIR
        inst.r2 = parse_registro(t[2]); // REG
    }
    else if (!strcmp(t[0],"RESIZE")) {
        inst.opcode = INST_RESIZE;
        inst.inmediato = atoi(t[1]);
    }
    else if (!strcmp(t[0],"COPY_STRING")) {
        inst.opcode = INST_COPY_STRING;
        inst.inmediato = atoi(t[1]);
    }
    else {
        inst.opcode = INST_EXIT;
    }

    string_array_destroy(t);
    return inst;
}
