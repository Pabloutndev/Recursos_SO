#include <interrupciones/interrupciones.h>

static volatile bool flag_interrupcion = false;

void interrupcion_init(void) {
    flag_interrupcion = false;
}

bool interrupcion_pendiente(void) {
    return flag_interrupcion;
}

void interrupcion_disparar(int tipo) {
    flag_interrupcion = true;
    //TODO:
    //kernel_enviar_interrupcion(tipo);
}

void interrupciones_init(void)
{
    interrupcion_init();
}