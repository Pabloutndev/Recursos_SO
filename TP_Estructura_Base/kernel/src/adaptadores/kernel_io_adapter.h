#ifndef KERNEL_IO_ADAPTER_H
#define KERNEL_IO_ADAPTER_H

#include <model/model.h>
#include <pcb/pcb.h>
#include <paquete/paquete.h>
#include <stdbool.h>
#include <stdint.h>

/* ========================================
 * ADAPTADOR: Kernel → IO
 * 
 * Responsabilidad:
 * - Convertir eventos internos de Kernel a requests de IO
 * - Enviar requests usando protocolo/mensajes
 * - Cada tipo de IO (Sleep, FS, etc.) tiene su adaptador
 * ======================================== */

/* Transformación de estructuras */
t_io_sleep* pcb_a_io_sleep(t_pcb* pcb, uint32_t tiempo);

/* OPERACIONES: Kernel solicita operación IO a interfaz */

/**
 * kernel_sleep
 * Kernel solicita que proceso duerma X ms.
 * 
 * El flujo es:
 * 1. Kernel bloquea el proceso
 * 2. Envía SLEEP a IO
 * 3. IO ejecuta sleep y avisa cuando termina
 * 4. Kernel desbloquea proceso
 */
void kernel_sleep(t_pcb* pcb, uint32_t tiempo_ms, 
                                              char* interfaz_io);

/**
 * kernel_fs_operation
 * Kernel solicita operación FS a interfaz IO.
 * Parametrizado: tipo operación (CREATE, DELETE, READ, WRITE, TRUNCATE)
 */
void kernel_fs_operation(t_pcb* pcb, 
                                                     const char* tipo_operacion,
                                                     const char* nombre_archivo,
                                                     uint32_t tamanio,
                                                     char* interfaz_io);

/**
 * kernel_io_adapter_atender_fin_operacion
 * IO notifica que terminó su tarea.
 */
void kernel_io_adapter_atender_fin_operacion(int fd, t_paquete* p);

#endif
