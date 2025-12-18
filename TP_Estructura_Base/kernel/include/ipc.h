// ipc.h  (mensajes entre procesos) (inter-process communication)
#ifndef IPC_H
#define IPC_H

#include <stdint.h>

typedef enum {
    OP_HANDSHAKE,
    OP_CREATE_PROCESS,
    OP_DISPATCH,       // enviar contexto al CPU
    OP_INTERRUPT,      // desalojar proceso
    OP_CONTEXT_REPLY,  // CPU -> Kernel: contexto actualizado + motivo
    OP_MEM_INIT,       // Kernel <-> Memoria
    OP_OK,
    OP_ERROR,
    // Master/Worker/Storage specific ops can be added
} op_code_t;

#endif
