#include <ciclo_instruccion/ciclo.h>
#include <ciclo_instruccion/decode.h>
#include <instrucciones/instrucciones.h>
#include <instrucciones/operaciones.h>
#include <mmu/mmu.h>
#include <cpu.h>
#include <model/model.h>
#include <stdlib.h>
#include <unistd.h>
#include <interrupciones/interrupciones.h>

void ciclo_instruccion_ejecutar(t_contexto_cpu* ctx) {    

    if (!ctx) {
        log_error(CPU_CTX.logger_error, "Contexto NULL en ciclo de instruccion cpu");
        return;
    }

    log_info(CPU_CTX.logger, "CPU ejecutando PID %u", ctx->pid);
    
    // MMU: Una vez por cambio de estado
    mmu_set_contexto(ctx);
       
    while (true) {

        if (interrupcion_pendiente()) {
            log_info(CPU_CTX.logger, "CPU PID %u: Interrupción detectada", ctx->pid);
            break;
        }

        char* linea = fetch_instruccion(ctx);
        if (!linea) {
            log_info(CPU_CTX.logger, "CPU PID %u: Fin de archivo/instrucciones", ctx->pid);
            break;
        }

        instruccion_t inst = decode_instruccion(ctx, linea);        
        free(linea); 

        CPU_CTX.motivo_desalojo = execute_instruccion(&inst, ctx);
        
        if (CPU_CTX.motivo_desalojo != CPU_CONTINUAR) {
            break;
        }
    }
}

char* fetch_instruccion(t_contexto_cpu* ctx)
{
    uint32_t pc = ctx->pc;
    // MMU translation removed as Memory handles PC -> Instruction logic
    
    char* linea = memoria_fetch_instruccion(ctx->pid, pc);

    log_info(CPU_CTX.logger, "PID: %u - FETCH - Program Counter: %u", ctx->pid, pc);

    return linea;
}

instruccion_t decode_instruccion(t_contexto_cpu* ctx, const char* linea)
{
    char* linea_copia = strdup(linea);

    if (!linea_copia) {
        log_error(CPU_CTX.logger, "Error duplicando instrucción");
        return (instruccion_t){ .opcode = INST_EXIT };
    }

    instruccion_t inst = decoder_parsear(linea_copia);

    log_info(CPU_CTX.logger, "PID: %u - DECODE - Instruccion: %s", ctx->pid, linea);

    free(linea_copia);
    return inst;
}
