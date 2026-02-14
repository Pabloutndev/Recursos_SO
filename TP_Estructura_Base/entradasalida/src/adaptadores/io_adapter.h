#ifndef IO_ADAPTER_H_
#define IO_ADAPTER_H_

#include <paquete/paquete.h>

void io_adapter_atender_sleep(int fd, t_paquete* p);
void io_adapter_atender_stdin_read(int fd, t_paquete* p);
void io_adapter_atender_stdout_write(int fd, t_paquete* p);
void io_adapter_atender_fs_create(int fd, t_paquete* p);
void io_adapter_atender_fs_delete(int fd, t_paquete* p);
void io_adapter_atender_fs_truncate(int fd, t_paquete* p);
void io_adapter_atender_fs_write(int fd, t_paquete* p);
void io_adapter_atender_fs_read(int fd, t_paquete* p);

#endif
