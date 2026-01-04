#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include <stdint.h>
#include <stdbool.h>

/* Gestión de Segmentos */
bool segmentacion_crear_proceso(uint32_t pid, int size_bytes);
void segmentacion_destruir_proceso(uint32_t pid);

/* Traducción */
/* En segmentación se traduce (PID, Segmento, Offset). 
   Pero la interfaz CPU -> Memoria suele ser (PID, DirLogica) y Memoria resuelve, 
   o CPU envía (Seg, Off) si CPU sabe de segmentación.
   Para una Memoria "Universal", si CPU solo manda Dirección Lógica Lineal,
   Memoria debe decodificarla. O si CPU manda componentes.
   Asumiremos interfaz genérica: traducir_direccion(pid, dir_logica)
*/
uint32_t segmentacion_traducir(uint32_t pid, uint32_t dir_logica);

#endif
