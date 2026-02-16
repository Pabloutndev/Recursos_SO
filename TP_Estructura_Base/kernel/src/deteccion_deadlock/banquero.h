#ifndef BANQUERO_H
#define BANQUERO_H

#include <stdbool.h>

// Verifica si el estado actual del sistema es seguro.
// Retorna true si hay una secuencia segura (no hay deadlock potencial).
// Retorna false si no se puede encontrar secuencia segura.
bool banquero_estado_seguro(void);

#endif
