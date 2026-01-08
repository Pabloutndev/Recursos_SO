#ifndef COMMON_KERNEL_CPU_H
#define COMMON_KERNEL_CPU_H

typedef enum {
    MOTIVO_FIN_QUANTUM,
    MOTIVO_IO,
    MOTIVO_EXIT,
    MOTIVO_SEGFAULT,
    MOTIVO_DESALOJO
} t_motivo_desalojo;

#endif
