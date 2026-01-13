
#ifndef SERIALIZACION_IO_H
#define SERIALIZACION_IO_H

#include <paquete/paquete.h>
#include <common/io/io_ops.h>
#include <protocolo/op_code.h>

/// ##### REQUESTS - SERIALIZACION #####

t_paquete* serializar_io_sleep(const t_io_sleep* req);
t_io_sleep* deserializar_io_sleep(t_paquete* p);

t_paquete* serializar_io_fs_write(const t_io_fs_write* req);
t_io_fs_write* deserializar_io_fs_write(t_paquete* p);

t_paquete* serializar_io_fs_create(const t_io_fs_create* req);
t_io_fs_create* deserializar_io_fs_create(t_paquete* p);

#endif /* SERIALIZACION_IO_H */
