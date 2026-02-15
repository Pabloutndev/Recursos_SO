#include <gestion/esquema_memoria.h>
#include <gestion/paginas.h>
#include <gestion/segmentacion.h>
#include <mod_memoria.h>
#include <commons/log.h>

extern t_log* logger;

static t_esquema esquema_activo = ESQUEMA_PAGINACION;

void esquema_memoria_init(t_esquema esquema) {
    esquema_activo = esquema;
    log_info(logger, "ESQUEMA MEMORIA: Inicializado en modo %s",
             esquema == ESQUEMA_PAGINACION ? "PAGINACION" : "SEGMENTACION");
}

t_esquema esquema_memoria_actual(void) { return esquema_activo; }

bool esquema_crear_proceso(uint32_t pid, uint32_t tamanio) {
    if (esquema_activo == ESQUEMA_PAGINACION)
        return paginacion_crear_proceso(pid, tamanio);
    else
        return segmentacion_crear_proceso(pid, tamanio);
}

void esquema_destruir_proceso(uint32_t pid) {
    if (esquema_activo == ESQUEMA_PAGINACION)
        paginacion_destruir_proceso(pid);
    else
        segmentacion_destruir_proceso(pid);
}

bool esquema_resize(uint32_t pid, uint32_t nuevo_tamanio) {
    if (esquema_activo == ESQUEMA_PAGINACION)
        return paginacion_resize(pid, nuevo_tamanio);
    else
        return segmentacion_resize(pid, nuevo_tamanio);
}

bool esquema_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    if (esquema_activo == ESQUEMA_PAGINACION)
        return paginacion_escribir(pid, dir_logica, buffer, size);
    else
        return segmentacion_escribir(pid, dir_logica, buffer, size);
}

bool esquema_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    if (esquema_activo == ESQUEMA_PAGINACION)
        return paginacion_leer(pid, dir_logica, buffer, size);
    else
        return segmentacion_leer(pid, dir_logica, buffer, size);
}

char* esquema_leer_instruccion(uint32_t pid, uint32_t pc) {
    if (esquema_activo == ESQUEMA_PAGINACION)
        return paginacion_leer_instruccion(pid, pc);
    else
        return segmentacion_leer_instruccion(pid, pc);
}

int64_t esquema_traducir(uint32_t pid, uint32_t dir_logica) {
    if (esquema_activo == ESQUEMA_PAGINACION) {
        // In paginacion, get page entry and compute physical address
        uint32_t pagina = dir_logica / MEMORIA_CTX.config->tam_pagina;
        uint32_t offset = dir_logica % MEMORIA_CTX.config->tam_pagina;
        t_pagina* pag = paginacion_obtener_entrada(pid, pagina);
        if (!pag || !pag->presente) return -1;
        return (int64_t)(pag->frame * MEMORIA_CTX.config->tam_pagina + offset);
    } else {
        uint32_t resultado = segmentacion_traducir(pid, dir_logica);
        if (resultado == 0) return -1;
        return (int64_t)resultado;
    }
}
