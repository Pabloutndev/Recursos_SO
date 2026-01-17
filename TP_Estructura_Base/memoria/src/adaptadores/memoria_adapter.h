#ifndef MEMORIA_ADAPTER_H
#define MEMORIA_ADAPTER_H

#include <model/model.h>

/* ========================================
 * ADAPTADOR: Memoria - Procesamiento de Requests
 * 
 * Responsabilidad:
 * - Recibir requests (paquetes deserializados)
 * - Llamar lógica interna de Memoria
 * - Enviar respuestas apropiadas
 * ======================================== */

/* REQUEST HANDLERS (reciben estructura + socket para responder) */

/**
 * memoria_adapter_init_proceso
 * Kernel solicita crear nuevo proceso en Memoria.
 * 
 * @param req: Request con PID y tamaño
 * @param socket_kernel: Para enviar respuesta (OK/FAIL)
 */
void memoria_adapter_init_proceso(t_mem_init_proceso* req, int socket_kernel);

/**
 * memoria_adapter_fin_proceso
 * Kernel notifica fin de proceso.
 * 
 * @param req: Request con PID
 * @param socket_kernel: Socket (sin respuesta esperada)
 */
void memoria_adapter_fin_proceso(t_mem_fin_proceso* req, int socket_kernel);

/**
 * memoria_adapter_traducir_pagina
 * CPU solicita traducción dirección lógica → física.
 * 
 * @param req: Request con PID + dirección lógica
 * @param socket_cpu: Para enviar respuesta (marco + ok)
 */
void memoria_adapter_traducir_pagina(t_mem_traducir* req, int socket_cpu);

/**
 * memoria_adapter_fetch_instruccion
 * CPU solicita instrucción en dirección específica.
 * 
 * @param req: Request con PID + PC
 * @param socket_cpu: Para enviar instrucción
 */
void memoria_adapter_fetch_instruccion(t_mem_fetch* req, int socket_cpu);

/**
 * memoria_adapter_leer
 * CPU solicita lectura de memoria.
 * 
 * @param req: Request con PID + dirección + tamaño
 * @param socket_cpu: Para enviar datos leídos
 */
void memoria_adapter_leer(t_mem_read* req, int socket_cpu);

/**
 * memoria_adapter_escribir
 * CPU solicita escritura en memoria.
 * 
 * @param req: Request con PID + dirección + datos
 * @param socket_cpu: Para enviar confirmación (OK/FAIL)
 */
void memoria_adapter_escribir(t_mem_write* req, int socket_cpu);

#endif
