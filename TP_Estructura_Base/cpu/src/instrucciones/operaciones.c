#include <instrucciones/operaciones.h>
#include <registros/registros.h>
#include <mmu/mmu.h>
#include <conexiones/cpu_memoria.h>
#include <protocolo/op_code.h>
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
            // MOV_IN (Registro, Dirección Lógica)
            // Lee de memoria (Dir Logica) y guarda en Registro
            {
                uint32_t dir_logica = inst->inmediato; // Asumiendo inmediato es la dir logica (o leer de registro segun arquitectura)
                // Usualmente MOV_IN R1, [DIR] -> R1 recibe valor de memoria. 
                // Revisar struct instruccion_t. Asumimos R1=Dest, Inmediato=Indir/Dir? 
                // En C-Code base comun: MOV_IN R1, R2 (donde R2 tiene la dir logica)
                uint32_t dir_logica_val = registros_leer(&ctx->registros, inst->r2); 
                
                uint32_t dir_fisica = mmu_traducir(dir_logica_val, false);
                if (dir_fisica > 0) { // 0 as error?
                    uint32_t valor_leido = 0;
                    if (memoria_leer(ctx->pid, dir_fisica, &valor_leido, sizeof(uint32_t))) {
                        registros_escribir(&ctx->registros, inst->r1, valor_leido);
                         ctx->registros.PC++;
                    } else {
                        // Error lectura
                        ctx->motivo_desalojo = INST_EXIT; // O Error memoria
                        strcpy(ctx->parametros, "SEG_FAULT_READ"); 
                        ctx->bloqueado = 1;
                    }
                } else {
                    // Page Fault (ya manejado en mmu_traducir? MMU devuelve 0 on error, y manda log error)
                    // Si MMU falló (PF o error), deberia desalojar?
                    // mmu_traducir deberia setear flag de desalojo? 
                    // Revisar mmu.c. MMU devuelve 0.
                    ctx->motivo_desalojo = OP_SEGFAULT; // Usamos OpCode como motivo?
                    ctx->bloqueado = 1;
                }
            }
            break;
        case INST_MOV_OUT:
            // MOV_OUT (Dirección Lógica, Registro)
            // Escribe valor de Registro en Memoria (Dir Logica)
            {
                uint32_t dir_logica_val = registros_leer(&ctx->registros, inst->r1);
                uint32_t valor_escribir = registros_leer(&ctx->registros, inst->r2);

                uint32_t dir_fisica = mmu_traducir(dir_logica_val, true);

                if (dir_fisica > 0) {
                    if (memoria_escribir(ctx->pid, dir_fisica, &valor_escribir, sizeof(uint32_t))) {
                         ctx->registros.PC++;
                    } else {
                        ctx->motivo_desalojo = INST_EXIT; 
                        strcpy(ctx->parametros, "SEG_FAULT_WRITE");
                        ctx->bloqueado = 1;
                    }
                } else {
                    ctx->motivo_desalojo = OP_SEGFAULT;
                    ctx->bloqueado = 1;
                }
            }
            break;
        case INST_RESIZE:
             // TODO: memoria resize request (enviar a Memoria resize, esperar respuesta)
             // Ajustar tamaño proceso.
            break;
        case INST_COPY_STRING:
            // Complejo: Leer string de memoria (SI) y escribir en memoria (DI).
            // Requiere loop de lectura escritura byte a byte o bloque.
            // Por simplicidad del snippet, dejamos TODO o implementamos basico.
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
