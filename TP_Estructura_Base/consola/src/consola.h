#ifndef CONSOLA_STANDALONE_H
#define CONSOLA_STANDALONE_H

#include <stdbool.h>
#include <commons/log.h>

void consola_init(const char* config_path);
void consola_run(void);
void consola_shutdown(void);

#endif
