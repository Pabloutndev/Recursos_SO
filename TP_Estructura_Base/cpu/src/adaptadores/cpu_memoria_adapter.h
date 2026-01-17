#ifndef CPU_MEMORIA_ADAPTER_H
#define CPU_MEMORIA_ADAPTER_H

#include <common/memoria/memoria.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================
 * ADAPTADOR: CPU ↔ Memoria
 * 
 * Responsabilidad:
 * - Convertir solicitudes de CPU a requests de Memoria
 * - Enviar requests y procesar responses sincrónicamente
 * - CPU necesita respuestas inmediatas para continuar
 * ======================================== */

/* Transformación de estructuras */
t_mem_fetch* cpu_a_mem_fetch(uint32_t pid, uint32_t pc);
t_mem_traducir* cpu_a_mem_traduccion(uint32_t pid, uint32_t pagina);
t_mem_read* cpu_a_mem_read(uint32_t pid, uint32_t direccion, uint32_t size);
t_mem_write* cpu_a_mem_write(uint32_t pid, uint32_t direccion, 
                              void* buffer, uint32_t size);

/* OPERACIONES COMPLETAS (Request + Response Síncrono) */

/**
 * cpu_fetch_instruccion
 * 
 * CPU solicita instrucción en dirección PC para un proceso.
 * SÍNCRONO: Espera respuesta de Memoria.
 * 
 * @param pid: Identificador del proceso
 * @param pc: Program Counter (dirección lógica)
 * @return Instrucción como string, NULL si error
 */
char* cpu_fetch_instruccion(uint32_t pid, uint32_t pc);

/**
 * cpu_traducir_pagina
 * 
 * CPU solicita traducción: dirección lógica → físico (marco)
 * SÍNCRONO: Espera traducción de Memoria.
 * 
 * @param pid: Identificador del proceso
 * @param pagina: Número de página (dirección lógica)
 * @param marco: OUT - Marco traducido
 * @return true si traducción exitosa, false si error/page fault
 */
bool cpu_traducir_pagina(uint32_t pid, uint32_t pagina, 
                         uint32_t* marco);

/**
 * cpu_leer
 * 
 * CPU solicita lectura de memoria en dirección física.
 * SÍNCRONO: Espera datos de Memoria.
 * 
 * @param pid: Identificador del proceso
 * @param dir_fisica: Dirección física (ya traducida)
 * @param buffer: OUT - Datos leídos
 * @param size: Cantidad de bytes a leer
 * @return true si lectura exitosa, false si error
 */
bool cpu_leer(uint32_t pid, uint32_t dir_fisica, 
              void* buffer, uint32_t size);

/**
 * cpu_escribir
 * 
 * CPU solicita escritura en memoria en dirección física.
 * SÍNCRONO: Espera confirmación de Memoria.
 * 
 * @param pid: Identificador del proceso
 * @param dir_fisica: Dirección física (ya traducida)
 * @param buffer: Datos a escribir
 * @param size: Cantidad de bytes
 * @return true si escritura exitosa, false si error
 */
bool cpu_escribir(uint32_t pid, uint32_t dir_fisica, 
                  void* buffer, uint32_t size);

/* INICIALIZACIÓN */
int cpu_memoria_adapter_init(int fd_mem);

#endif
