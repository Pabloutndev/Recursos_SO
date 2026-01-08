#ifndef COMMON_IO_OPS_H
#define COMMON_IO_OPS_H

#include <common/tipos_basicos.h>

typedef struct {
    pid_t pid;
    uint32_t tiempo;
} t_io_sleep;

typedef struct {
    pid_t pid;
    char path[256];
    uint32_t offset;
    uint32_t size;
} t_io_fs_write;

typedef struct {
    pid_t pid;
    char path[256];
} t_io_fs_create;

#endif
