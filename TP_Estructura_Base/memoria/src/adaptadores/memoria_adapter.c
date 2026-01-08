#include "memoria_adapter.h"

#include "memoria_adapter.h"
#include <memoria/estructuras.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>

///TODO: REVISAR
void manejar_init_proceso(t_mem_init_proceso* req)
{
    // 1. Crear estructura del proceso
    t_tabla_paginas* tabla = tabla_paginas_crear(req->pid, req->tamanio);

    // 2. Reservar frames necesarios
    reservar_frames(tabla);

    // 3. Registrar en memoria global
    dictionary_put(tablas_por_pid,
                   string_itoa(req->pid),
                   tabla);
}

void manejar_fin_proceso(t_mem_fin_proceso* req)
{
    char* key = string_itoa(req->pid);

    t_tabla_paginas* tabla =
        dictionary_remove(tablas_por_pid, key);

    if (tabla != NULL) {
        liberar_frames(tabla);
        tabla_paginas_destruir(tabla);
    }

    free(key);
}
