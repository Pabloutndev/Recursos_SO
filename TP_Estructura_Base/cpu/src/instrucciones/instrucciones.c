#include <instrucciones/instrucciones.h>
#include <ciclo_instruccion/decode.h>
#include <instrucciones/operaciones.h>
#include <conexiones/cpu_memoria.h>
#include <model/model.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

bool ejecutar_siguiente_instruccion(t_contexto_cpu* ctx)
{
    char* linea = memoria_fetch_instruccion(ctx->pid, ctx->registros.PC);
    if (!linea) {
        return false;
    }

    instruccion_t inst = (instruccion_t) decoder_parsear(linea);
    free(linea);
    
    if (inst.opcode==INST_EXIT) {
        return false;
    }

    execute_instruccion(&inst, ctx);
    return true;
}
