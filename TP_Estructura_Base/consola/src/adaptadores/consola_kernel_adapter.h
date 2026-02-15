#ifndef CONSOLA_KERNEL_ADAPTER_H
#define CONSOLA_KERNEL_ADAPTER_H

#include <stdint.h>
#include <protocolo/op_code.h>

void enviar_comando_string(int socket, op_code code, const char* str);
void enviar_comando_uint32(int socket, op_code code, uint32_t val);
void enviar_comando_simple(int socket, op_code code);

#endif
