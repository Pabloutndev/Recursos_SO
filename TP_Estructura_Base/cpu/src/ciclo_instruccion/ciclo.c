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
    /*
     * Ciclo de Instrucción: Fetch → Decode → Execute
     * 
     * Puntos de salida:
     * 1. ctx->finalizado = 1      [Instrucción EXIT o EOF]
     * 2. ctx->bloqueado = 1       [Instrucción IO, WAIT, etc.]
     * 3. interrupcion_pendiente() [Señal de quantum/desalojo]
     * 4. fetch_instruccion() NULL [Fin de archivo]
     * 
     * Responsabilidades:
     * - Validar que contexto es válido
     * - Detectar interrupciones tempranamente
     * - Manejar errores de fetch gracefully
     * - Mantener estado consistente al salir
     */

    if (!ctx) {
        log_error(logger, "Contexto NULL en ciclo");
        return;
    }

    mmu_set_contexto(ctx);

    if (!(ctx->finalizado || ctx->bloqueado)) {
        log_info(logger, "CPU ejecutando PID %d", ctx->pid);
    }
    
    while (!(ctx->finalizado || ctx->bloqueado)) {

        // ✅ PUNTO CRÍTICO: Detección temprana de interrupción
        if (interrupcion_pendiente()) {
            log_info(logger, "CPU PID %d: Interrupción detectada", ctx->pid);
            break;  // Salir del ciclo para atender interrupcion
        }

        char* linea = fetch_instruccion(ctx);
        if (!linea) {
            log_info(logger, "CPU PID %d: Fin de archivo/instrucciones", ctx->pid);
            ctx->finalizado = 1;
            break;  // Error fetch o fin de programa
        }

        instruccion_t inst = decode_instruccion(linea);
        
        execute_instruccion(&inst, ctx);
        free(linea); 

        // Retardo opcional para debugging/visualización
        // if (CPU_CONF.retardo_instruccion > 0) {
        //     usleep(CPU_CONF.retardo_instruccion);
        // }

        // ✅ Quantum check (gestionado por Kernel principalmente)
        // Este contador es respaldo en caso de que el timer del Kernel no funcione
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
