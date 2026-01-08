#include <instrucciones/operaciones.h>
#include <registros/registros.h>
#include <stdbool.h>

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
        case INST_SET: 
            ejecutar_set(inst, ctx);
            break;
        case INST_SUM: 
            ejecutar_sum(inst, ctx);
            break;
        case INST_SUB: 
            ejecutar_sub(inst, ctx);
            break;
        case INST_JNZ: 
            ejecutar_jnz(inst, ctx);
            break;
        case INST_IO: 
            ejecutar_io(inst, ctx);
            break;
        case INST_EXIT: 
            ejecutar_exit(ctx);
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
    // Convención clásica del TP:
    // si el último resultado ≠ 0 → salto
    if (registros_leer(&ctx->registros, REG_PC) != 0) {
        ctx->registros.PC = i->inmediato;
    } else {
        ctx->registros.PC++;
    }
}

static void ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx)
{
    ctx->bloqueado = true;
    ctx->io_time = i->inmediato;

    // NO incrementa PC → kernel guarda contexto
}

static void ejecutar_exit(t_contexto_cpu* ctx)
{
    ctx->finalizado = true;
}
