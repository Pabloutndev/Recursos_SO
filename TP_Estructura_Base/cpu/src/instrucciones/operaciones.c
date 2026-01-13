#include <instrucciones/operaciones.h>
#include <registros/registros.h>
#include <stdbool.h>
#include <string.h>

static void ejecutar_set(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sum(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sub(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_jnz(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_exit(t_contexto_cpu* ctx);

void execute_instruccion(instruccion_t* inst, void* contexto)
{
    t_contexto_cpu* ctx = (t_contexto_cpu*) contexto;

    switch (inst->opcode) {
        case INST_SET: ejecutar_set(inst, ctx); break;
        case INST_SUM: ejecutar_sum(inst, ctx); break;
        case INST_SUB: ejecutar_sub(inst, ctx); break;
        case INST_JNZ: ejecutar_jnz(inst, ctx); break;
        case INST_EXIT: ejecutar_exit(ctx); break;
        
        // Desalojos
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
            // Generic handler for eviction
            // IMPORTANTE: Incrementar PC antes de desalojar para que al volver
            // ejecute la SIGUIENTE instrucción.
            ctx->registros.PC++;
            
            ctx->motivo_desalojo = (uint8_t) inst->opcode;
            strcpy(ctx->parametros, inst->parametros);
            ctx->bloqueado = 1; // Stop cycle
            break;

        case INST_MOV_IN:
            // TODO: mmu translate + read
            // ejecutar_mov_in(inst, ctx);
            break;
        case INST_MOV_OUT:
            // TODO: mmu translate + write
            // ejecutar_mov_out(inst, ctx);
            break;
        case INST_RESIZE:
            // TODO: memoria resize request
            break;
        case INST_COPY_STRING:
            // TODO: string copy
            break;
        default:
            break;
    }
}

static void ejecutar_set(instruccion_t* i, t_contexto_cpu* ctx)
{
    registros_escribir(&ctx->registros, i->r1, i->inmediato);
    ctx->registros.PC++;
}

static void ejecutar_sum(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t a = registros_leer(&ctx->registros, i->r1);
    uint32_t b = registros_leer(&ctx->registros, i->r2);
    registros_escribir(&ctx->registros, i->r1, a + b);
    ctx->registros.PC++;
}

static void ejecutar_sub(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t a = registros_leer(&ctx->registros, i->r1);
    uint32_t b = registros_leer(&ctx->registros, i->r2);
    registros_escribir(&ctx->registros, i->r1, a - b);
    ctx->registros.PC++;
}

static void ejecutar_jnz(instruccion_t* i, t_contexto_cpu* ctx)
{
    // Fix: Validar registro != 0
    if (registros_leer(&ctx->registros, i->r1) != 0) {
        ctx->registros.PC = i->inmediato;
    } else {
        ctx->registros.PC++;
    }
}

static void ejecutar_exit(t_contexto_cpu* ctx)
{
    ctx->finalizado = true;
    ctx->motivo_desalojo = INST_EXIT; // Redundante pero util
}
