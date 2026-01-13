#ifndef CPU_MEMORIA_H
#define CPU_MEMORIA_H

#include <stdint.h>

void cpu_conexiones_memoria_init(char* ip, char* puerto);
void cpu_conexiones_memoria_close(void);

// Solicita instruccion a memoria dado un PC (Program Counter)
char* memoria_fetch_instruccion(uint32_t pid, uint32_t pc);

// Retorna frame/marco para una pagina dada
bool memoria_obtener_marco(uint32_t pid, uint32_t pagina, bool escritura, uint32_t* marco);

#endif
