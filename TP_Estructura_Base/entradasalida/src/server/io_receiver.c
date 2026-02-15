#include <stdlib.h>
#include <commons/log.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>
#include <conexion/conexion.h>
#include <adaptadores/io_adapter.h>
#include <core/io_main.h>

void io_receiver_loop() {
    log_info(IO_CTX.logger, "IO: escuchando Kernel");
    while (1) {
        t_paquete* paquete = recibir_paquete(IO_CTX.socket_kernel);
        if (paquete == NULL) {
            log_error(IO_CTX.logger, "IO: Kernel desconectado");
            break;
        }

        log_info(IO_CTX.logger, "IO: recibido op=%d", paquete->codigo_operacion);

        switch (paquete->codigo_operacion) {
            case OP_HANDSHAKE:
                log_info(IO_CTX.logger, "IO: handshake Kernel");
                handshake_servidor(IO_CTX.socket_kernel, OP_OK, IO_CTX.logger);
                break;

            case OP_IO_SLEEP:
                io_adapter_atender_sleep(IO_CTX.socket_kernel, paquete);
                break;

            case OP_IO_STDIN_READ:
                io_adapter_atender_stdin_read(IO_CTX.socket_kernel, paquete);
                break;

            case OP_IO_STDOUT_WRITE:
                io_adapter_atender_stdout_write(IO_CTX.socket_kernel, paquete);
                break;

            case OP_IO_FS_CREATE:
                io_adapter_atender_fs_create(IO_CTX.socket_kernel, paquete);
                break;

            case OP_IO_FS_DELETE:
                io_adapter_atender_fs_delete(IO_CTX.socket_kernel, paquete);
                break;

            case OP_IO_FS_TRUNCATE:
                io_adapter_atender_fs_truncate(IO_CTX.socket_kernel, paquete);
                break;

            case OP_IO_FS_WRITE:
                io_adapter_atender_fs_write(IO_CTX.socket_kernel, paquete);
                break;

            case OP_IO_FS_READ:
                io_adapter_atender_fs_read(IO_CTX.socket_kernel, paquete);
                break;

            default:
                log_warning(IO_CTX.logger, "IO: operacion desconocida op=%d", paquete->codigo_operacion);
                enviar_respuesta_fail(IO_CTX.socket_kernel);
                break;
        }

        paquete_destroy(paquete);
    }
}
