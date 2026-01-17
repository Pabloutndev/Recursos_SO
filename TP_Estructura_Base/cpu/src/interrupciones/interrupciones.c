#include <interrupciones/interrupciones.h>

/*
 * Sistema de Interrupciones del CPU
 * 
 * Responsable: Flag volátil compartido entre hilos
 * - handler_interrupt(): Setea flag cuando recibe OP_INTERRUPCION_CPU
 * - ciclo_instruccion_ejecutar(): Lee flag cada iteración
 * 
 * Garantías de sincronización:
 * - volatile bool es atómico en arquitecturas x86-64
 * - No requiere mutex por ser lectura/escritura de bool simple
 * - Flag se setea asincronamente desde handler_interrupt()
 * - Se resetea sincronamente en handler_dispatch()
 */

static volatile bool flag_interrupcion = false;

void interrupcion_init(void) {
    flag_interrupcion = false;
}

bool interrupcion_pendiente(void) {
    // ✅ Lectura atómica del flag
    // Usado en ciclo_instruccion_ejecutar() ANTES del FETCH
    return flag_interrupcion;
}

void interrupcion_disparar(int tipo) {
    // ✅ Seteo del flag desde handler_interrupt()
    // Llamado cuando Kernel envía OP_INTERRUPCION_CPU
    flag_interrupcion = true;
    // Nota: tipo no se usa actualmente, pero se mantiene para extensibilidad
}

void interrupciones_init(void)
{
    interrupcion_init();
}

void interrupcion_reset(void)
{
    // ✅ Reset seguro del flag desde handler_dispatch()
    // Llamado DESPUÉS de detectar que ciclo salió por interrupción
    flag_interrupcion = false;
}