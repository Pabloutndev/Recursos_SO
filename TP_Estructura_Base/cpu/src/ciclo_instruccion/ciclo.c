#include <ciclo_instruccion/ciclo.h>
#include <ciclo_instruccion/decode.h>
#include <instrucciones/instrucciones.h>
#include <instrucciones/operaciones.h>
#include <mmu/mmu.h>
#include <cpu.h>
#include <stdlib.h>
#include <unistd.h>
#include <interrupciones/interrupciones.h>

void ciclo_instruccion_ejecutar(t_contexto_cpu* ctx) {    

    mmu_set_contexto(ctx);

    if (!(ctx->finalizado || ctx->bloqueado)) {
        log_info(logger, "CPU ejecutando PID %d", ctx->pid);
    }
    
    while (!(ctx->finalizado || ctx->bloqueado)) {

        if (interrupcion_pendiente()) {
            break; // Salir del ciclo para atender interrupcion en handler
        }

        char* linea = fetch_instruccion(ctx);
        if (!linea) break; // Error fetch

        instruccion_t inst = decode_instruccion(linea);
        
        execute_instruccion(&inst, ctx);
        free(linea); 

        sleep(2); // Retardo a demanda

        // Quantum check (managed by CPU counter here)
        if (ctx->quantum > 0) {
             ctx->quantum--;
             if (ctx->quantum == 0) {
                 interrupcion_disparar(0); 
                 break;
             }
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
