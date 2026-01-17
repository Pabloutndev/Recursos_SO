#include <ciclo_instruccion/ciclo.h>
#include <ciclo_instruccion/decode.h>
#include <instrucciones/instrucciones.h>
#include <instrucciones/operaciones.h>
#include <mmu/mmu.h>
#include <cpu.h>
#include <cpu/contexto.h>
#include <stdlib.h>
#include <unistd.h>
#include <interrupciones/interrupciones.h>

extern t_log* loggerError;

void ciclo_instruccion_ejecutar(t_contexto_cpu* ctx) {    

    if (!ctx) {
        log_error(loggerError, "Contexto NULL en ciclo de instruccion cpu");
        return;
    }

    log_info(logger, "CPU ejecutando PID %d", ctx->pid);
    
    // MMU: Una vez por cambio de estado
    mmu_set_contexto(ctx);
       
    while (true) {

        if (interrupcion_pendiente()) {
            log_info(logger, "CPU PID %d: Interrupción detectada", ctx->pid);
            cpu_set_motivo(CPU_FIN_QUANTUM);
            break;
        }

        char* linea = fetch_instruccion(ctx);
        if (!linea) {
            log_info(logger, "CPU PID %d: Fin de archivo/instrucciones", ctx->pid);
            cpu_set_motivo(CPU_EXIT);
            break;
        }

        instruccion_t inst = decode_instruccion(linea);        
        free(linea); 

        t_cpu_motivo motivo = execute_instruccion(&inst, ctx);
        
        if (motivo != CPU_CONTINUAR) {
            cpu_set_motivo(motivo);
            break;
        }
    }
}

char* fetch_instruccion(t_contexto_cpu* ctx)
{
    uint32_t pc = ctx->registros.PC;
    // MMU translation removed as Memory handles PC -> Instruction logic
    
    char* linea = memoria_fetch_instruccion(ctx->pid, pc);

    log_info(logger, "FETCH PID %d PC %d -> %s",
             ctx->pid, pc, linea ? linea : "NULL");

    return linea;
}

instruccion_t decode_instruccion(const char* linea)
{
    char* linea_copia = strdup(linea);

    if (!linea_copia) {
        log_error(logger, "Error duplicando instrucción");
        return (instruccion_t){ .opcode = INST_EXIT };
    }

    instruccion_t inst = decoder_parsear(linea_copia);

    log_info(logger, "DECODE opcode %d", inst.opcode);

    free(linea_copia);
    return inst;
}
