#ifndef COMMON_MEMORIA_RESPONSES_H
#define COMMON_MEMORIA_RESPONSES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool ok;
    uint32_t frame;
} t_mem_respuesta_traduccion;

typedef struct {
    bool ok;
    void* data; 
    uint32_t size;
} t_mem_respuesta_lectura;



#endif
