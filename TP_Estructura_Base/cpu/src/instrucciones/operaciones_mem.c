#include <instrucciones/operaciones_internal.h>
#include <registros/registros.h>
#include <mmu/mmu.h>
#include <conexiones/cpu_memoria.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <paquete/paquete.h>
#include <stdlib.h>
#include <string.h>
#include <cpu.h>

extern t_log* logger;

t_motivo_desalojo ejecutar_mov_in(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t dir_logica = registros_leer(&ctx->registros, i->r2);
    uint32_t size = registros_size(i->r1);
    uint32_t dir_fisica = mmu_traducir(dir_logica, false);

    if (dir_fisica == TRADUCCION_ERROR) {
        return MOTIVO_SEGFAULT;
    }

    uint32_t valor_leido = 0;
    if (!memoria_leer(ctx->pid, dir_fisica, &valor_leido, size)) {
        return MOTIVO_SEGFAULT;
    }

    registros_escribir(&ctx->registros, i->r1, valor_leido);
    ctx->pc++;

    return CPU_CONTINUAR;
}

t_motivo_desalojo ejecutar_mov_out(instruccion_t* i, t_contexto_cpu* ctx)
{
    uint32_t dir_logica_val = registros_leer(&ctx->registros, i->r1);
    uint32_t size = registros_size(i->r2);
    uint32_t valor_escribir = registros_leer(&ctx->registros, i->r2);

    uint32_t dir_fisica = mmu_traducir(dir_logica_val, true);

    if (dir_fisica == TRADUCCION_ERROR) {
        return MOTIVO_SEGFAULT;
    }

    if (!memoria_escribir(ctx->pid, dir_fisica, &valor_escribir, size)) {
        return MOTIVO_SEGFAULT;
    }

    ctx->pc++;
    return CPU_CONTINUAR;
}

t_motivo_desalojo ejecutar_resize(instruccion_t* i, t_contexto_cpu* ctx)
{
    // RESIZE tamanio: Solicita a Memoria ajustar el tamaño del proceso
    uint32_t nuevo_tamanio = i->inmediato;

    log_info(logger, "PID: %u - RESIZE a %u bytes", ctx->pid, nuevo_tamanio);

    // Enviar solicitud de resize a memoria via adapter
    t_mem_resize req = { .pid = ctx->pid, .nuevo_tamanio = nuevo_tamanio };
    enviar_resize(CPU_CTX.socket_memoria, &req, OP_MEM_AJUSTAR_TAMANIO);

    // Esperar respuesta
    t_paquete* resp = recibir_paquete(CPU_CTX.socket_memoria);
    if (!resp || !recibir_respuesta(resp)) {
        log_error(logger, "PID: %u - RESIZE fallido (Out of Memory)", ctx->pid);
        if (resp) paquete_destroy(resp);
        return MOTIVO_SEGFAULT;
    }

    paquete_destroy(resp);
    ctx->pc++;
    return CPU_CONTINUAR;
}

t_motivo_desalojo ejecutar_copy_string(instruccion_t* i, t_contexto_cpu* ctx)
{
    // COPY_STRING tamanio: Copia tamanio bytes desde dir en SI a dir en DI
    uint32_t tamanio = i->inmediato;
    uint32_t dir_origen = registros_leer(&ctx->registros, REG_SI);
    uint32_t dir_destino = registros_leer(&ctx->registros, REG_DI);

    log_info(logger, "PID: %u - COPY_STRING %u bytes de %u a %u",
             ctx->pid, tamanio, dir_origen, dir_destino);

    // Traducir origen
    uint32_t df_origen = mmu_traducir(dir_origen, false);
    if (df_origen == TRADUCCION_ERROR) return MOTIVO_SEGFAULT;

    // Leer datos de origen
    void* buffer = malloc(tamanio);
    if (!buffer) return MOTIVO_SEGFAULT;

    if (!memoria_leer(ctx->pid, df_origen, buffer, tamanio)) {
        free(buffer);
        return MOTIVO_SEGFAULT;
    }

    // Traducir destino
    uint32_t df_destino = mmu_traducir(dir_destino, true);
    if (df_destino == TRADUCCION_ERROR) {
        free(buffer);
        return MOTIVO_SEGFAULT;
    }

    // Escribir datos en destino
    if (!memoria_escribir(ctx->pid, df_destino, buffer, tamanio)) {
        free(buffer);
        return MOTIVO_SEGFAULT;
    }

    free(buffer);
    ctx->pc++;
    return CPU_CONTINUAR;
}
