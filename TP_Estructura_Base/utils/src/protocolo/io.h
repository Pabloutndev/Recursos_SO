#ifndef PROTOCOLO_IO_H_
#define PROTOCOLO_IO_H_

#include <serializacion/io.h>
#include <common/cpu/contexto.h>
#include <common/cpu/tlb.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

/// ==============================
/// IO - SLEEP
/// ==============================
void enviar_io_sleep(int socket_io, t_io_sleep* io);
t_io_sleep* recibir_io_sleep(t_paquete* p);

// ================================
// IO - FS WRITE
// ================================
void enviar_io_fs_write(int socket_io, t_io_fs_write* io);
t_io_fs_write* recibir_io_fs_write(t_paquete* p);

// ================================
// IO - FS CREATE
// ================================
void enviar_io_fs_create(int socket_io, t_io_fs_create* io);
t_io_fs_create* recibir_io_fs_create(t_paquete* p);

// ================================
// FUNCIONES AUXILIARES
// ================================

void enviar_io_fin_ok(int socket_kernel, uint32_t pid);
void enviar_io_fin_fail(int socket_kernel, uint32_t pid);
uint32_t recibir_pid_fin_io(int socket_kernel);

#endif /* PROTOCOLO_IO_H_ */