
#ifndef KERNEL_CON_CPU_H
#define KERNEL_CON_CPU_H

#include <common/contexto.h>
#include <stdbool.h>

// Inicializa conexion con CPU (Dispatch y Interrupt)
void conectar_cpu(char* ip, char* puerto_dispatch, char* puerto_interrupt);

// Envía contexto a CPU Dispatch para ejecutar
void enviar_contexto_a_cpu(contexto_t* ctx);

// Envia interrupcion a CPU Interrupt
void enviar_interrupcion_a_cpu(int pid, int motivo);

// Recibe contexto devuelto por CPU
contexto_t* recibir_contexto_de_cpu(void);

#endif
