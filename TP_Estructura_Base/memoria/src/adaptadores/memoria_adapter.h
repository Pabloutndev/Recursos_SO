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

#include <paquete/paquete.h>

/* ATENCIÓN: Las funciones de adaptador ahora reciben el paquete y el socket
 * para centralizar la deserialización, lógica y respuesta. */

void memoria_adapter_atender_init_proceso(int fd, t_paquete* paquete);
void memoria_adapter_atender_fin_proceso(int fd, t_paquete* paquete);
void memoria_adapter_atender_traducir_pagina(int fd, t_paquete* paquete);
void memoria_adapter_atender_fetch_instruccion(int fd, t_paquete* paquete);
void memoria_adapter_atender_leer(int fd, t_paquete* paquete);
void memoria_adapter_atender_escribir(int fd, t_paquete* paquete);
void memoria_adapter_atender_resize(int fd, t_paquete* paquete);

#endif
