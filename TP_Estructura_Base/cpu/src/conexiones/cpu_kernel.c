#include <conexiones/cpu_kernel.h>
//#include <utils/conexion.h>

static int socket_kernel;

void conexiones_kernel_init(char* ip, char* puerto) {
    //socket_kernel = conectar(ip, puerto);
}

bool kernel_recibir_contexto(contexto_t* ctx) {
    //return recibir_contexto(socket_kernel, ctx);
}

void kernel_enviar_contexto(const contexto_t* ctx) {
    //enviar_contexto(socket_kernel, ctx);
}

void kernel_enviar_interrupcion(int tipo) {
    //enviar_interrupcion(socket_kernel, tipo);
}