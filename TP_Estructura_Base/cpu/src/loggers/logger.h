#ifndef LOGGER_CPU_H
#define LOGGER_CPU_H

#include <commons/log.h>
#include <instrucciones/instrucciones.h>
#include <contexto/contexto.h>
#include <stdint.h>

extern t_log* logger;

/// ==========================
/// LOGS OBLIGATORIOS CPU
/// ==========================

/// Ciclo de instrucción
void log_cpu_inicio_pid(int pid);
void log_cpu_fetch(int pid, uint32_t pc, const char* instruccion);
void log_cpu_decode(opcode_t opcode);
void log_cpu_execute(opcode_t opcode);

/// Instrucciones
void log_cpu_set(int pid, reg_id_t reg, uint32_t valor);
void log_cpu_sum(int pid, reg_id_t dst, reg_id_t src);
void log_cpu_sub(int pid, reg_id_t dst, reg_id_t src);
void log_cpu_jnz(int pid, uint32_t nuevo_pc);

/// Memoria / MMU
void log_cpu_mmu_traduccion(int pid, uint32_t dir_logica, uint32_t dir_fisica);
void log_cpu_page_fault(int pid, uint32_t pagina);

/// Interrupciones
void log_cpu_fin_quantum(int pid);
void log_cpu_io(int pid, uint32_t tiempo);
void log_cpu_exit(int pid);

/// Contexto
void log_cpu_contexto_enviado(int pid);

#endif /* LOGGER_CPU_H */
