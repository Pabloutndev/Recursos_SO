#include <gestion/reemplazo.h>
#include <commons/collections/dictionary.h>
#include <gestion/paginas.h>
#include <frames/frames.h>
#include <mod_memoria.h>
#include <model/model.h>

extern t_dictionary* tablas_paginas;

/* Clock hand: persists between calls so the algorithm resumes where it left off */
static int clock_hand = 0;

int elegir_victima_clock(uint32_t* pid_victima, int* pagina_victima)
{
    if (!tablas_paginas) return -1;

    int total_frames = frames_totales();
    if (total_frames <= 0) return -1;

    /* Wrap hand in case frame count changed */
    if (clock_hand >= total_frames) clock_hand = 0;

    /*
     * We need to scan all present pages to find who occupies each frame.
     * Build a lookup: frame_number -> (pid, page_number, t_pagina*)
     * Then walk the frames circularly starting from clock_hand.
     */
    typedef struct {
        uint32_t pid;
        int nro_pagina;
        t_pagina* pagina;
    } t_frame_owner;

    t_frame_owner* owners = calloc(total_frames, sizeof(t_frame_owner));
    if (!owners) return -1;

    /* Mark all as empty */
    for (int i = 0; i < total_frames; i++) {
        owners[i].pagina = NULL;
    }

    /* Populate the owner table from all page tables */
    t_list* keys = dictionary_keys(tablas_paginas);
    for (int k = 0; k < list_size(keys); k++) {
        char* key = list_get(keys, k);
        uint32_t pid = atoi(key);
        t_list* tabla = dictionary_get(tablas_paginas, key);
        if (!tabla) continue;

        for (int p = 0; p < list_size(tabla); p++) {
            t_pagina* pag = list_get(tabla, p);
            if (pag->presente && pag->frame >= 0 && pag->frame < total_frames) {
                owners[pag->frame].pid = pid;
                owners[pag->frame].nro_pagina = p;
                owners[pag->frame].pagina = pag;
            }
        }
    }
    list_destroy(keys);

    /*
     * Clock sweep: start at clock_hand, go around at most 2 full turns.
     * First turn: clear use bits (second chance). Second turn: victim guaranteed.
     */
    int scanned = 0;
    int limit = total_frames * 2;

    while (scanned < limit) {
        int idx = clock_hand;
        clock_hand = (clock_hand + 1) % total_frames;
        scanned++;

        t_frame_owner* owner = &owners[idx];
        if (!owner->pagina) continue; /* frame not occupied by any page */

        if (!owner->pagina->uso) {
            /* Found victim */
            *pid_victima = owner->pid;
            *pagina_victima = owner->nro_pagina;
            int frame = owner->pagina->frame;
            free(owners);
            return frame;
        }

        /* Give second chance: clear use bit, advance */
        owner->pagina->uso = false;
    }

    free(owners);
    log_error(MEMORIA_CTX.logger, "CLOCK: no se encontro pagina victima");
    return -1;
}
