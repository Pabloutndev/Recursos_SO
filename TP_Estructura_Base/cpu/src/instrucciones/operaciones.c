#include <instrucciones/operaciones.h>
#include <registros/registros.h>
#include <mmu/mmu.h>
#include <conexiones/cpu_memoria.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <paquete/paquete.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <cpu.h>

extern t_log* logger;
extern instruccion_t ultima_instruccion;

static void ejecutar_set(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sum(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sub(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_jnz(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_exit(t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_mov_out(instruccion_t* i, t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_mov_in(instruccion_t* i, t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_resize(instruccion_t* i, t_contexto_cpu* ctx);
static t_motivo_desalojo ejecutar_copy_string(instruccion_t* i, t_contexto_cpu* ctx);

t_motivo_desalojo execute_instruccion(instruccion_t* inst, t_contexto_cpu* ctx)
{
    log_info(logger, "PID: %u - Ejecutando: %d", ctx->pid, inst->opcode);
    ultima_instruccion = *inst;

    switch (inst->opcode) {
        case INST_SET: ejecutar_set(inst, ctx); return CPU_CONTINUAR;
        case INST_SUM: ejecutar_sum(inst, ctx); return CPU_CONTINUAR;
        case INST_SUB: ejecutar_sub(inst, ctx); return CPU_CONTINUAR;
        case INST_JNZ: ejecutar_jnz(inst, ctx); return CPU_CONTINUAR;
        case INST_EXIT: ejecutar_exit(ctx); return MOTIVO_EXIT;

        // Desalojos por I/O y recursos
        case INST_WAIT:
        case INST_SIGNAL:
        case INST_IO_GEN_SLEEP:
        case INST_IO_STDIN_READ:
        case INST_IO_STDOUT_WRITE:
        case INST_IO_FS_CREATE:
        case INST_IO_FS_DELETE:
        case INST_IO_FS_TRUNCATE:
        case INST_IO_FS_WRITE:
        case INST_IO_FS_READ:
            return ejecutar_io(inst, ctx);

        case INST_MOV_IN:
            return ejecutar_mov_in(inst, ctx);

        case INST_MOV_OUT:
            return ejecutar_mov_out(inst, ctx);

        case INST_RESIZE:
            return ejecutar_resize(inst, ctx);

        case INST_COPY_STRING:
            return ejecutar_copy_string(inst, ctx);

        default:
            return CPU_CONTINUAR;
    }
}

static const char* reg_nombre(reg_id_t r) {
    switch(r) {
        case REG_AX: return "AX"; case REG_BX: return "BX";
        case REG_CX: return "CX"; case REG_DX: return "DX";
        case REG_EAX: return "EAX"; case REG_EBX: return "EBX";
        case REG_ECX: return "ECX"; case REG_EDX: return "EDX";
        case REG_SI: return "SI"; case REG_DI: return "DI";
        default: return "?";
    }
}

static void ejecutar_set(instruccion_t* i, t_contexto_cpu* ctx)
{
    registros_escribir(&ctx->registros, i->r1, i->inmediato);
    log_info(logger, "PID: %u - SET %s = %u", ctx->pid, reg_nombre(i->r1), i->inmediato);
    ctx->pc++;
}

static void ejecutar_sum(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t a = registros_leer(&ctx->registros, i->r1);
    uint32_t b = registros_leer(&ctx->registros, i->r2);
    uint32_t resultado = a + b;
    registros_escribir(&ctx->registros, i->r1, resultado);
    log_info(logger, "PID: %u - SUM %s(%u) + %s(%u) = %u", ctx->pid, reg_nombre(i->r1), a, reg_nombre(i->r2), b, resultado);
    ctx->pc++;
}

static void ejecutar_sub(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t a = registros_leer(&ctx->registros, i->r1);
    uint32_t b = registros_leer(&ctx->registros, i->r2);
    uint32_t resultado = a - b;
    registros_escribir(&ctx->registros, i->r1, resultado);
    log_info(logger, "PID: %u - SUB %s(%u) - %s(%u) = %u", ctx->pid, reg_nombre(i->r1), a, reg_nombre(i->r2), b, resultado);
    ctx->pc++;
}

static void ejecutar_jnz(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t val = registros_leer(&ctx->registros, i->r1);
    if (val != 0) {
        log_info(logger, "PID: %u - JNZ %s=%u != 0 -> salto a PC=%u", ctx->pid, reg_nombre(i->r1), val, i->inmediato);
        ctx->pc = i->inmediato;
    } else {
        log_info(logger, "PID: %u - JNZ %s=0 -> continua PC=%u", ctx->pid, reg_nombre(i->r1), ctx->pc + 1);
        ctx->pc++;
    }
}

static t_motivo_desalojo ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx)
{
    ctx->pc++;
    return MOTIVO_IO;
}

static t_motivo_desalojo ejecutar_mov_in(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t dir_logica = registros_leer(&ctx->registros, i->r2);
    uint32_t size = registros_size(i->r1);
    uint32_t dir_fisica = mmu_traducir(dir_logica, false);

    if (dir_fisica == TRADUCCION_ERROR) {
        return MOTIVO_SEGFAULT;
    }

    uint32_t valor_leido = 0;
    if (!memoria_leer(ctx->pid, dir_fisica, &valor_leido, size)) {
        return MOTIVO_SEGFAULT;
    }

    registros_escribir(&ctx->registros, i->r1, valor_leido);
    ctx->pc++;

    return CPU_CONTINUAR;
}

static t_motivo_desalojo ejecutar_mov_out(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t dir_logica_val = registros_leer(&ctx->registros, i->r1);
    uint32_t size = registros_size(i->r2);
    uint32_t valor_escribir = registros_leer(&ctx->registros, i->r2);

    uint32_t dir_fisica = mmu_traducir(dir_logica_val, true);

    if (dir_fisica == TRADUCCION_ERROR) {
        return MOTIVO_SEGFAULT;
    }

    if (!memoria_escribir(ctx->pid, dir_fisica, &valor_escribir, size)) {
        return MOTIVO_SEGFAULT;
    }

    ctx->pc++;
    return CPU_CONTINUAR;
}

static t_motivo_desalojo ejecutar_resize(instruccion_t* i, t_contexto_cpu* ctx)
{
    // RESIZE tamanio: Solicita a Memoria ajustar el tamaño del proceso
    uint32_t nuevo_tamanio = i->inmediato;

    log_info(logger, "PID: %u - RESIZE a %u bytes", ctx->pid, nuevo_tamanio);

    // Enviar solicitud de resize a memoria via adapter
    t_mem_resize req = { .pid = ctx->pid, .nuevo_tamanio = nuevo_tamanio };
    enviar_resize(CPU_CTX.socket_memoria, &req, OP_MEM_AJUSTAR_TAMANIO);

    // Esperar respuesta
    t_paquete* resp = recibir_paquete(CPU_CTX.socket_memoria);
    if (!resp || !recibir_respuesta(resp)) {
        log_error(logger, "PID: %u - RESIZE fallido (Out of Memory)", ctx->pid);
        if (resp) paquete_destroy(resp);
        return MOTIVO_SEGFAULT;
    }

    paquete_destroy(resp);
    ctx->pc++;
    return CPU_CONTINUAR;
}

static t_motivo_desalojo ejecutar_copy_string(instruccion_t* i, t_contexto_cpu* ctx)
{
    // COPY_STRING tamanio: Copia tamanio bytes desde dir en SI a dir en DI
    uint32_t tamanio = i->inmediato;
    uint32_t dir_origen = registros_leer(&ctx->registros, REG_SI);
    uint32_t dir_destino = registros_leer(&ctx->registros, REG_DI);

    log_info(logger, "PID: %u - COPY_STRING %u bytes de %u a %u",
             ctx->pid, tamanio, dir_origen, dir_destino);

    // Traducir origen
    uint32_t df_origen = mmu_traducir(dir_origen, false);
    if (df_origen == TRADUCCION_ERROR) return MOTIVO_SEGFAULT;

    // Leer datos de origen
    void* buffer = malloc(tamanio);
    if (!buffer) return MOTIVO_SEGFAULT;

    if (!memoria_leer(ctx->pid, df_origen, buffer, tamanio)) {
        free(buffer);
        return MOTIVO_SEGFAULT;
    }

    // Traducir destino
    uint32_t df_destino = mmu_traducir(dir_destino, true);
    if (df_destino == TRADUCCION_ERROR) {
        free(buffer);
        return MOTIVO_SEGFAULT;
    }

    // Escribir datos en destino
    if (!memoria_escribir(ctx->pid, df_destino, buffer, tamanio)) {
        free(buffer);
        return MOTIVO_SEGFAULT;
    }

    free(buffer);
    ctx->pc++;
    return CPU_CONTINUAR;
}

static void ejecutar_exit(t_contexto_cpu* ctx)  {
    log_info(logger, "PID: %u - EXIT", ctx->pid);
}
