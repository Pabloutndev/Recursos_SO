#include <serializacion/io.h>
#include <common/cpu/contexto.h>
#include <common/cpu/tlb.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>

// ================= Kernel -> IO =================

/// ==============================
/// IO - SLEEP
/// ==============================
void enviar_io_sleep(int socket_io, t_io_sleep* io)
{
    t_paquete* p = serializar_io_sleep((const t_io_sleep*) io);
    enviar_paquete(socket_io, p);
    eliminar_paquete(p);
}

t_io_sleep* recibir_io_sleep(t_paquete* p)
{
    return deserializar_io_sleep(p);
}

// ================================
// IO - FS WRITE
// ================================
void enviar_io_fs_write(int socket_io, t_io_fs_write* io)
{
    t_paquete* p = serializar_io_fs_write((const t_io_fs_write*) io);
    enviar_paquete(socket_io, p);
    eliminar_paquete(p);
}

t_io_fs_write* recibir_io_fs_write(t_paquete* p)
{
    return deserializar_io_fs_write(p);
}

// ================================
// IO - FS CREATE
// ================================
void enviar_io_fs_create(int socket_io, t_io_fs_create* io)
{
    t_paquete* p = serializar_io_fs_create((const t_io_fs_create*) io);
    enviar_paquete(socket_io, p);
    eliminar_paquete(p);
}

t_io_fs_create* recibir_io_fs_create(t_paquete* p)
{
    return deserializar_io_fs_create(p);
}

// ================================
// FUNCIONES AUXILIARES
// ================================

void enviar_io_fin_ok(int socket_kernel, uint32_t pid)
{
    t_paquete* p = crear_paquete(OP_IO_FIN);
    agregar_entero_a_paquete(p, pid);
    enviar_paquete(socket_kernel, p);
    eliminar_paquete(p);
}

void enviar_io_fin_fail(int socket_kernel, uint32_t pid)
{
    t_paquete* p = crear_paquete(OP_FAIL);
    agregar_entero_a_paquete(p, pid);
    enviar_paquete(socket_kernel, p);
    eliminar_paquete(p);
}

uint32_t recibir_pid_fin_io(int socket_kernel)
{
    t_list* valores = recibir_paquete(socket_kernel);
    uint32_t pid = *(uint32_t*)list_get(valores, 0);
    list_destroy_and_destroy_elements(valores, free);
    return pid;
}