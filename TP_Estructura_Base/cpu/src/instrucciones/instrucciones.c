#include <instrucciones/instrucciones.h>
#include <ciclo_instruccion/decode.h>
#include <instrucciones/operaciones.h>
#include <conexiones/cpu_memoria.h>
#include <stddef.h>
#include <contexto/contexto.h>
#include <stdbool.h>

bool ejecutar_siguiente_instruccion(t_contexto_cpu* ctx)
{
    char* linea = memoria_fetch_instruccion(ctx->pid, ctx->registros.PC);
    if (!linea) {
        ctx->finalizado = 1;
        return false;
    }

    instruccion_t inst = (instruccion_t) decoder_parsear(linea);
    free(linea);
    
    if (inst.opcode==INST_EXIT) {
        ctx->finalizado = 1;
        return false;
    }

    execute_instruccion(&inst, ctx);
    return true;
}
