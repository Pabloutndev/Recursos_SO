#include <gestion/reemplazo.h>
#include <commons/collections/dictionary.h>
#include <gestion/paginas.h>
#include <mod_memoria.h>
#include <model/model.h>

static uint32_t clock_pid_victima;
static int clock_pagina_victima;
static int clock_frame_victima;
static bool clock_encontrada;
extern t_dictionary* tablas_paginas;

static void clock_buscar_victima(char* key, void* value)
{
    if (clock_encontrada) return;

    uint32_t pid = atoi(key);
    t_list* tabla = (t_list*) value;

    for (int i = 0; i < list_size(tabla); i++) {
        t_pagina* p = list_get(tabla, i);

        if (!p->presente) continue;

        if (!p->uso) {
            clock_pid_victima = pid;
            clock_pagina_victima = i;
            clock_frame_victima = p->frame;
            clock_encontrada = true;
            return;
        }

        // Segunda oportunidad
        p->uso = false;
    }
}

int elegir_victima_clock(uint32_t* pid_victima, int* pagina_victima)
{
    if (!tablas_paginas) return -1;

    clock_encontrada = false;
    clock_frame_victima = -1;

    dictionary_iterator(tablas_paginas, clock_buscar_victima);

    if (!clock_encontrada) {
        dictionary_iterator(tablas_paginas, clock_buscar_victima);
    }

    if (!clock_encontrada) {
        log_error(MEMORIA_CTX.logger, "CLOCK: no se encontró página víctima");
        return -1;
    }

    *pid_victima = clock_pid_victima;
    *pagina_victima = clock_pagina_victima;

    return clock_frame_victima;
}
