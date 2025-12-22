#include <ciclo_instruccion/ciclo.h>
#include <ciclo_instruccion/decode.h>
#include <instrucciones/instrucciones.h>
#include <instrucciones/operaciones.h>
#include <mmu/mmu.h>
#include <cpu.h>
#include <unistd.h>

void ciclo_instruccion_ejecutar(contexto_t* ctx) {    

    mmu_set_contexto(ctx);

    if (!(ctx->finalizado || ctx->bloqueado)) {
        log_info(logger, "CPU ejecutando PID %d", ctx->pid);
    }
    
    while (!(ctx->finalizado || ctx->bloqueado)) {

        char* linea = fetch_instruccion(ctx);

        instruccion_t inst = decode_instruccion(linea);
        
        execute_instruccion(&inst, ctx);

        sleep(2);

        if ((--ctx->quantum) <= 0) {
            log_info(logger, "Fin quantum PID %d", ctx->pid);
            kernel_enviar_interrupcion(FIN_QUANTUM);
            break;
        }
    }
}

char* fetch_instruccion(contexto_t* ctx)
{
    uint32_t pc = ctx->registros.PC;
    uint32_t dir_fisica = mmu_traducir(pc, false);

    char* linea = memoria_leer_instruccion(ctx->pid, dir_fisica);

    log_info(logger, "FETCH PID %d PC %d -> %s",
             ctx->pid, pc, linea);

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
