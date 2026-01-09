#include <adaptadores/memoria_adapter.h>

#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>

#include <configs/memoria_config.h>
#include <gestion/memoria_core.h>
#include <frames/frames.h>
#include <gestion/paginas.h>
#include <common/memoria/requests.h>
#include <common/memoria/responses.h>

#include <protocolo/protocolo_cpu_memoria.h>

extern t_log* logger;
extern t_memoria_config* memoria_config;

/* Kernel → Memoria */
void manejar_init_proceso(t_mem_init_proceso* req)
{
    log_info(logger,
        "MEMORIA: INIT_PROCESO PID=%u TAM=%u",
        req->pid, req->tamanio);
    
    bool ok = paginacion_crear_proceso(req->pid, req->tamanio);

    if (!ok) {
        log_warning(logger,
            "MEMORIA: PID %u ya existente", req->pid);
    }
}

void manejar_fin_proceso(t_mem_fin_proceso* req)
{
    log_info(logger,
        "MEMORIA: FIN_PROCESO PID=%u", req->pid);

    paginacion_destruir_proceso(req->pid);
}

/* CPU → Memoria */
void manejar_traduccion_pagina(t_mem_traducir_pagina* req, int socket_cpu)
{
    t_mem_respuesta_traduccion resp = {
        .ok = false,
        .frame = 0
    };

    t_pagina* pag =
        paginacion_obtener_entrada(req->pid, req->pagina);

    if (!pag) {
        log_error(logger,
            "MEMORIA: Pagina invalida PID=%u PAG=%u",
            req->pid, req->pagina);

        enviar_respuesta_traduccion(socket_cpu, &resp);
        return;
    }

    if (!pag->presente) {
        // PAGE FAULT
        int frame = obtener_frame_libre();

        if (frame < 0) {
            log_error(logger,
                "MEMORIA: Sin frames libres (PID=%u)",
                req->pid);

            enviar_respuesta_traduccion(socket_cpu, &resp);
            return;
        }

        pag->frame = frame;
        pag->presente = true;
        pag->uso = true;
        pag->modificado = false;
    }

    resp.ok = true;
    resp.frame = pag->frame;

    enviar_respuesta_traduccion(socket_cpu, &resp);
}


void manejar_lectura_memoria(t_mem_rw* req, int socket_cpu)
{
    t_mem_respuesta_lectura resp = {
        .ok = false,
        .data = NULL,
        .size = req->tamanio
    };
    
    int pagina = req->direccion_fisica / memoria_config->tam_pagina;
    int offset = req->direccion_fisica % memoria_config->tam_pagina;

    t_pagina* pag = paginacion_obtener_entrada(req->pid, pagina);

    if (!pag || !pag->presente) {
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    uint32_t dir_fisica = 
        pag->frame * memoria_config->tam_pagina + offset;

    void* buffer = malloc(req->tamanio);
    if (!memoria_leer(dir_fisica, req->tamanio, buffer)) {
        free(buffer);
        enviar_respuesta_lectura(socket_cpu, &resp);
        return;
    }

    resp.ok = true;
    resp.data = buffer;

    enviar_respuesta_lectura(socket_cpu, &resp);
    free(buffer);
}

void manejar_escritura_memoria(t_mem_rw* req, int socket_cpu)
{
    /*memcpy(
        memoria_fisica + req->direccion_fisica,
        req->data,
        req->tamanio
    );*/

    // marcar página como modificada
    int pagina = req->direccion_fisica / memoria_config->tam_pagina;
    t_pagina* pag =
        paginacion_obtener_entrada(req->pid, pagina);

    if (pag) {
        pag->modificado = true;
        pag->uso = true;
    }
}