#ifndef GRAFO_ESPERA_H
#define GRAFO_ESPERA_H

#include <stdbool.h>

// Construye un grafo de espera entre procesos y detecta ciclos con DFS.
// Retorna true si se detecto un deadlock (ciclo en el grafo).
// Loguea los PIDs involucrados en el ciclo via log_deadlock_detectado().
bool grafo_espera_detectar_deadlock(void);

#endif
