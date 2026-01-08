#ifndef PROTOCOLO_KERNEL_IO_H
#define PROTOCOLO_KERNEL_IO_H

#include <common/io/io_ops.h>

// Kernel -> IO
void enviar_io_sleep(int socket_io, t_io_sleep* io);
void enviar_io_fs_write(int socket_io, t_io_fs_write* io);
void enviar_io_fs_create(int socket_io, t_io_fs_create* io);

// IO -> Kernel
void enviar_io_fin_ok(int socket_kernel, uint32_t pid);
void enviar_io_fin_fail(int socket_kernel, uint32_t pid);

uint32_t recibir_pid_fin_io(int socket_kernel);

#endif