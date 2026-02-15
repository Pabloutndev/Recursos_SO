// consola_handler.c - Maneja conexiones de consola remota al kernel
#include <conexiones/consola_handler.h>
#include <mod_kernel.h>
#include <peticiones/proceso.h>
#include <peticiones/interrupciones.h>
#include <planificacion/planificacion.h>
#include <loggers/logger.h>

#include <conexion/conexion.h>
#include <paquete/paquete.h>
#include <protocolo/op_code.h>
#include <protocolo/mensajes.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <commons/string.h>

void* handler_consola_connection(void* arg)
{
    int fd = *(int*)arg;
    free(arg);

    // Handshake: esperar OP_HANDSHAKE y responder OP_OK
    t_paquete* hs = recibir_paquete(fd);
    if (!hs) {
        log_error(KERNEL_CTX.logger_error, "Consola: conexion cerrada antes de handshake FD=%d", fd);
        close(fd);
        return NULL;
    }

    if (hs->codigo_operacion != OP_HANDSHAKE) {
        log_error(KERNEL_CTX.logger_error, "Consola: handshake invalido opcode=%d FD=%d", hs->codigo_operacion, fd);
        paquete_destroy(hs);
        close(fd);
        return NULL;
    }
    paquete_destroy(hs);

    // Enviar OK
    enviar_respuesta_ok(fd);
    log_info(KERNEL_CTX.logger, "Consola: conectada FD=%d", fd);

    // Loop de atencion a comandos de la consola
    while (1) {
        t_paquete* msg = recibir_paquete(fd);
        if (!msg) {
            log_info(KERNEL_CTX.logger, "Consola: desconectada FD=%d", fd);
            break;
        }

        switch (msg->codigo_operacion) {

            case OP_CONSOLA_RUN: {
                char* path = paquete_read_string(msg);
                if (path) {
                    log_info(KERNEL_CTX.logger, "Consola: RUN %s", path);
                    ejecutar_proceso(path);
                    free(path);
                } else {
                    log_error(KERNEL_CTX.logger_error, "Consola: RUN sin argumento path");
                }
                break;
            }

            case OP_CONSOLA_KILL: {
                uint32_t pid = 0;
                paquete_read_uint32(msg, &pid);
                log_info(KERNEL_CTX.logger, "Consola: KILL PID=%u", pid);
                matar_proceso((int)pid);
                break;
            }

            case OP_CONSOLA_PS: {
                log_info(KERNEL_CTX.logger, "Consola: PS");

                // Construir texto con el estado de las colas (con mutex)
                pthread_mutex_lock(&mutex_new);
                char* pids_new = lista_pids(cola_new);
                pthread_mutex_unlock(&mutex_new);

                pthread_mutex_lock(&mutex_ready);
                char* pids_ready = lista_pids(cola_ready);
                pthread_mutex_unlock(&mutex_ready);

                pthread_mutex_lock(&mutex_exec);
                char* pids_exec = lista_pids(cola_exec);
                pthread_mutex_unlock(&mutex_exec);

                pthread_mutex_lock(&mutex_blocked);
                char* pids_blocked = lista_pids(cola_blocked);
                pthread_mutex_unlock(&mutex_blocked);

                pthread_mutex_lock(&mutex_exit);
                char* pids_exit = lista_pids(cola_exit);
                pthread_mutex_unlock(&mutex_exit);

                char* texto = string_new();
                string_append_with_format(&texto, "Estado: NEW     - Procesos: [%s]\n", pids_new ? pids_new : "");
                string_append_with_format(&texto, "Estado: READY   - Procesos: [%s]\n", pids_ready ? pids_ready : "");
                string_append_with_format(&texto, "Estado: EXEC    - Procesos: [%s]\n", pids_exec ? pids_exec : "");
                string_append_with_format(&texto, "Estado: BLOCKED - Procesos: [%s]\n", pids_blocked ? pids_blocked : "");
                string_append_with_format(&texto, "Estado: EXIT    - Procesos: [%s]", pids_exit ? pids_exit : "");

                // Enviar respuesta
                t_paquete* resp = paquete_create(OP_CONSOLA_PS_RESP);
                paquete_write_string(resp, texto);
                enviar_paquete(fd, resp);
                paquete_destroy(resp);

                // Limpiar
                if (pids_new) free(pids_new);
                if (pids_ready) free(pids_ready);
                if (pids_exec) free(pids_exec);
                if (pids_blocked) free(pids_blocked);
                if (pids_exit) free(pids_exit);
                free(texto);

                // Tambien logueamos en el kernel
                listar_procesos_por_estado();
                break;
            }

            case OP_CONSOLA_ALGORITMO: {
                char* algo_str = paquete_read_string(msg);
                if (algo_str) {
                    log_info(KERNEL_CTX.logger, "Consola: ALGORITMO %s", algo_str);

                    // Convertir a uppercase para comparacion robusta
                    char* upper = string_duplicate(algo_str);
                    string_to_upper(upper);

                    if (strcmp(upper, "FIFO") == 0)
                        set_algoritmo(ALG_FIFO);
                    else if (strcmp(upper, "RR") == 0)
                        set_algoritmo(ALG_RR);
                    else if (strcmp(upper, "VRR") == 0)
                        set_algoritmo(ALG_VRR);
                    else if (strcmp(upper, "HRRN") == 0)
                        set_algoritmo(ALG_HRRN);
                    else
                        log_error(KERNEL_CTX.logger_error, "Consola: algoritmo no valido %s", algo_str);

                    free(upper);
                    free(algo_str);
                } else {
                    log_error(KERNEL_CTX.logger_error, "Consola: ALGORITMO sin argumento valor");
                }
                break;
            }

            case OP_CONSOLA_START:
                log_info(KERNEL_CTX.logger, "Consola: START");
                planificacion_start();
                break;

            case OP_CONSOLA_PAUSE:
                log_info(KERNEL_CTX.logger, "Consola: PAUSE");
                planificacion_pause();
                break;

            case OP_CONSOLA_DESALOJAR: {
                uint32_t pid = 0;
                paquete_read_uint32(msg, &pid);
                log_info(KERNEL_CTX.logger, "Consola: DESALOJAR PID=%u", pid);
                desalojar_proceso(pid);
                break;
            }

            case OP_CONSOLA_EXIT:
                log_info(KERNEL_CTX.logger, "Consola: EXIT FD=%d", fd);
                paquete_destroy(msg);
                close(fd);
                return NULL;

            default:
                log_warning(KERNEL_CTX.logger, "Consola: opcode desconocido %d", msg->codigo_operacion);
                break;
        }

        paquete_destroy(msg);
    }

    close(fd);
    return NULL;
}
