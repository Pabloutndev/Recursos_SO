#include "protocolo_kernel_io.h"
#include <paquete/paquete.h>

// ================= Kernel -> IO =================

void enviar_io_sleep(int socket_io, t_io_sleep* io)
{
    t_paquete* p = crear_paquete(IO_GENERIC_SLEEP);

    agregar_entero_a_paquete(p, io->pid);
    agregar_entero_a_paquete(p, io->tiempo);

    enviar_paquete(p, socket_io);
    eliminar_paquete(p);
}

void enviar_io_fs_write(int socket_io, t_io_fs_write* io)
{
    t_paquete* p = crear_paquete(IO_FS_WRITE);

    agregar_entero_a_paquete(p, io->pid);
    agregar_a_paquete(p, io->path, sizeof(io->path));
    agregar_entero_a_paquete(p, io->offset);
    agregar_entero_a_paquete(p, io->size);

    enviar_paquete(p, socket_io);
    eliminar_paquete(p);
}

void enviar_io_fs_create(int socket_io, t_io_fs_create* io)
{
    t_paquete* p = crear_paquete(IO_FS_CREATE);

    agregar_entero_a_paquete(p, io->pid);
    agregar_a_paquete(p, io->path, sizeof(io->path));

    enviar_paquete(p, socket_io);
    eliminar_paquete(p);
}

// ================= IO -> Kernel =================

void enviar_io_fin_ok(int socket_kernel, uint32_t pid)
{
    t_paquete* p = crear_paquete(IO_FIN);
    agregar_entero_a_paquete(p, pid);
    enviar_paquete(p, socket_kernel);
    eliminar_paquete(p);
}

void enviar_io_fin_fail(int socket_kernel, uint32_t pid)
{
    t_paquete* p = crear_paquete(FAIL);
    agregar_entero_a_paquete(p, pid);
    enviar_paquete(p, socket_kernel);
    eliminar_paquete(p);
}

uint32_t recibir_pid_fin_io(int socket_kernel)
{
    t_list* valores = recibir_paquete(socket_kernel);
    uint32_t pid = *(uint32_t*)list_get(valores, 0);
    list_destroy_and_destroy_elements(valores, free);
    return pid;
}