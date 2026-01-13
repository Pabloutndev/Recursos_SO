
#ifndef CPU_CONTEXTO_H_
#define CPU_CONTEXTO_H_

#include <stdint.h>
#include <stdbool.h>
#include <common/registros.h>

typedef struct {
    uint32_t pid;
    uint32_t pc;
    uint32_t quantum;
    uint32_t finalizado;
    uint32_t bloqueado;
    uint32_t io_time;
    registros_t registros;
} t_contexto_cpu;

// Estructura para representar un proceso
typedef struct {
    int pid;
    int quantum;
    registros_t registros;
} t_process;

#endif /* CPU_CONTEXTO_H_ */
