#include <gestion/reemplazo.h>
#include <commons/collections/dictionary.h>
#include <gestion/paginas.h>
#include <mod_memoria.h>
#include <common/memoria/responses.h>

static uint32_t clock_pid = 0;
static int clock_pagina = 0;

int elegir_victima_clock(uint32_t* pid_victima, int* pagina_victima)
{
    /*
    //t_dictionary_iterator it = dictionary_iterator(tablas_paginas);

    while (dictionary_iterator_has_next(&it)) {
        char* key;
        t_list* tabla;
        dictionary_iterator_next(&it, &key, (void**)&tabla);

        uint32_t pid = atoi(key);

        for (int i = 0; i < list_size(tabla); i++) {
            t_pagina* pag = list_get(tabla, i);

            if (!pag->presente) continue;

            if (!pag->uso) {
                *pid_victima = pid;
                *pagina_victima = i;
                return pag->frame;
            }

            pag->uso = false;
        }
    }
    return -1; // No debería pasar
    */
}
