#include <mod_memoria.h>
#include <gestion/paginas.h>
#include <gestion/paginas_internal.h>
#include <gestion/memoria_ram.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

bool paginacion_escribir(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    uint32_t tam_pag = MEMORIA_CTX.config->tam_pagina;
    uint32_t bytes_escritos = 0;
    while (bytes_escritos < size) {
        uint32_t dir_actual = dir_logica + bytes_escritos;
        uint32_t num_pagina = dir_actual / tam_pag;
        uint32_t offset = dir_actual % tam_pag;
        uint32_t a_escribir = (tam_pag - offset < size - bytes_escritos) ? (tam_pag - offset) : (size - bytes_escritos);

        t_pagina* pag = paginacion_obtener_entrada(pid, num_pagina);
        if (!pag || !pag->presente) return false;

        pthread_mutex_lock(&mutex_paginas);
        /* Re-validate: page could have been evicted between obtener_entrada unlock and this lock */
        if (!pag->presente) {
            pthread_mutex_unlock(&mutex_paginas);
            continue; /* retry this chunk */
        }
        escribir_memoria_fisica(pag->frame * tam_pag + offset, (char*)buffer + bytes_escritos, a_escribir);
        pag->uso = true;
        pag->modificado = true;
        pthread_mutex_unlock(&mutex_paginas);

        bytes_escritos += a_escribir;
    }
    return true;
}

bool paginacion_leer(uint32_t pid, uint32_t dir_logica, void* buffer, uint32_t size) {
    uint32_t tam_pag = MEMORIA_CTX.config->tam_pagina;
    uint32_t bytes_leidos = 0;
    while (bytes_leidos < size) {
        uint32_t dir_actual = dir_logica + bytes_leidos;
        uint32_t num_pagina = dir_actual / tam_pag;
        uint32_t offset = dir_actual % tam_pag;
        uint32_t a_leer = (tam_pag - offset < size - bytes_leidos) ? (tam_pag - offset) : (size - bytes_leidos);

        t_pagina* pag = paginacion_obtener_entrada(pid, num_pagina);
        if (!pag || !pag->presente) return false;

        pthread_mutex_lock(&mutex_paginas);
        /* Re-validate: page could have been evicted between obtener_entrada unlock and this lock */
        if (!pag->presente) {
            pthread_mutex_unlock(&mutex_paginas);
            continue; /* retry this chunk */
        }
        leer_memoria_fisica(pag->frame * tam_pag + offset, (char*)buffer + bytes_leidos, a_leer);
        pag->uso = true;
        pthread_mutex_unlock(&mutex_paginas);

        bytes_leidos += a_leer;
    }
    return true;
}

char* paginacion_leer_instruccion(uint32_t pid, uint32_t pc)
{
    int tam_pag = MEMORIA_CTX.config->tam_pagina;

    // UNIFICACION: El PC es un indice, lo convertimos a direccion logica (byte offset)
    uint32_t dir_logica = pc * 64;

    int pag_nro = dir_logica / tam_pag;
    int offset  = dir_logica % tam_pag;

    t_pagina* pag = paginacion_obtener_entrada(pid, pag_nro);

    if (pag && pag->presente && pag->frame != -1) {
        pthread_mutex_lock(&mutex_paginas);
        /* Re-validate: page could have been evicted between obtener_entrada unlock and this lock */
        if (!pag->presente || pag->frame == -1) {
            pthread_mutex_unlock(&mutex_paginas);
            return NULL;
        }
        uint32_t dir_fisica = (pag->frame * tam_pag) + offset;

        char buffer[256];
        leer_memoria_fisica(dir_fisica, buffer, 255);
        buffer[255] = '\0';
        pag->uso = true;
        pthread_mutex_unlock(&mutex_paginas);

        return strdup(buffer);
    }

    return NULL;
}
