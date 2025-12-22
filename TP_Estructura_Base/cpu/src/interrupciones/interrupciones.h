#ifndef INTERRUPCIONES_H
#define INTERRUPCIONES_H

#include <stdbool.h>

void interrupciones_init(void);
bool interrupcion_pendiente(void);
void interrupcion_disparar(int tipo);

#endif
