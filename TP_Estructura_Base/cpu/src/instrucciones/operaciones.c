#include <instrucciones/operaciones.h>
#include <registros/registros.h>
#include <mmu/mmu.h>
#include <conexiones/cpu_memoria.h>
#include <protocolo/op_code.h>
#include <stdbool.h>
#include <string.h>
#include <cpu.h>

static void ejecutar_set(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sum(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_sub(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_jnz(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx);
static void ejecutar_exit(t_contexto_cpu* ctx);

t_cpu_motivo execute_instruccion(instruccion_t* inst, t_contexto_cpu* ctx)
{
    switch (inst->opcode) {
        case INST_SET: ejecutar_set(inst, ctx); return CPU_CONTINUAR; break;
        case INST_SUM: ejecutar_sum(inst, ctx); return CPU_CONTINUAR; break;
        case INST_SUB: ejecutar_sub(inst, ctx); return CPU_CONTINUAR; break;
        case INST_JNZ: ejecutar_jnz(inst, ctx); return CPU_CONTINUAR; break;
        case INST_EXIT: ejecutar_exit(ctx); return CPU_EXIT; break;
        
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
            return ejecutar_io(inst, ctx);

        case INST_MOV_IN:
            return ejecutar_mov_in(inst, ctx);
            
        case INST_MOV_OUT:
            return ejecutar_mov_out(inst, ctx);

        case INST_RESIZE:
             // TODO: memoria resize request (enviar a Memoria resize, esperar respuesta)
             // Ajustar tamaño proceso.
            return CPU_IO;

        case INST_COPY_STRING:
            // Complejo: Leer string de memoria (SI) y escribir en memoria (DI).
            // Requiere loop de lectura escritura byte a byte o bloque.
            // Por simplicidad del snippet, dejamos TODO o implementamos basico.
            return CPU_IO;
            
        default:
            return CPU_CONTINUAR;
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

static t_cpu_motivo ejecutar_io(instruccion_t* i, t_contexto_cpu* ctx)
{
    ctx->registros.PC++;
    strcpy(ctx->parametros, i->parametros);
    return CPU_IO;
}

static t_cpu_motivo ejecutar_mov_in(instruccion_t* i, t_contexto_cpu* ctx)
{
    // MOV_IN (Registro, Dirección Lógica)
    // Lee de memoria (Dir Logica) y guarda en Registro
    uint32_t dir_logica = registros_leer(&ctx->registros, i->r2);
    uint32_t dir_fisica = mmu_traducir(dir_logica, false);
    
    if (!dir_fisica) {
        return CPU_SEGFAULT;
    }

    uint32_t valor_leido = 0;
    if (!memoria_leer(ctx->pid, dir_fisica, &valor_leido, sizeof(uint32_t))) {
        // Page Fault (ya manejado en mmu_traducir? MMU devuelve 0 on error, y manda log error)
        // Si MMU falló (PF o error), deberia desalojar?
        // mmu_traducir deberia setear flag de desalojo? 
        // Revisar mmu.c. MMU devuelve 0.
        return CPU_SEGFAULT;
    }
    
    registros_escribir(&ctx->registros, inst->r1, valor_leido);
    ctx->registros.PC++;
    
    return CPU_CONTINUAR;
}

static t_cpu_motivo ejecutar_mov_out(instruccion_t* i, t_contexto_cpu* ctx)
{
    // MOV_OUT (Dirección Lógica, Registro)
    // Escribe valor de Registro en Memoria (Dir Logica)
    uint32_t dir_logica_val = registros_leer(&ctx->registros, i->r1);
    uint32_t valor_escribir = registros_leer(&ctx->registros, i->r2);

    uint32_t dir_fisica = mmu_traducir(dir_logica_val, true);

    if (!dir_fisica) {
        return CPU_SEGFAULT;
    }

    if (!memoria_escribir(ctx->pid, dir_fisica, &valor_escribir, sizeof(uint32_t))) {
        return CPU_SEGFAULT;
    }
    
    ctx->registros.PC++;
    return CPU_CONTINUAR;
}