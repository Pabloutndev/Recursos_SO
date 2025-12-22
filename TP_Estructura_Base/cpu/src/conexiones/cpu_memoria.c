#include <conexiones/cpu_memoria.h>
#include <loggers/logger.h>
#include <stdlib.h>
#include <string.h>

static char* programa[] = {
    "SET AX 5",
    "SET BX 3",
    "SUM AX BX",
    "SUB AX BX",
    "EXIT"
};

char* memoria_leer_instruccion(uint32_t pid, uint32_t dir_fisica)
{
    ///TODO:
    //  - serializa request
    //  - envía a Memoria
    //  - recibe string
    
    /// TODO: FIX: simulacion
    static char* programa[] = {
        "SET AX 5",
        "SET BX 3",
        "SUM AX BX",
        "EXIT",
        NULL
    };
    
    if (programa[dir_fisica] == NULL) {
        return NULL;
    }
    
    return strdup(programa[dir_fisica]);
    
    /* FIX:
    log_info(logger,
        "MEMORIA -> Leer instrucción PID %d DIR %u",
        pid, dir_fisica
    );

    // Simulación temporal
    return strdup("SUM AX BX");*/
}
