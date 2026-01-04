#ifndef FRAMES_H_
#define FRAMES_H_

#include <stdint.h>
#include <stdbool.h>

/* Inicialización del Bitmap de Frames */
int frames_init(void);
void frames_destroy(void);

/* Gestión de Frames */
int obtener_frame_libre(void);
void liberar_frame(int frame);

/* Consultas */
bool frame_esta_libre(int frame);
int frames_totales(void);
int frames_ocupados(void);

#endif
