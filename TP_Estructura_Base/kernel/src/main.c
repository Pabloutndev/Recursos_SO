// main.c
#include <mod_kernel.h>
#include <planificacion/planificacion.h>
#include <consola/consola.h>

#include <commons/log.h>
#include <signal.h>
#include <stdlib.h>

int main(int argc, char** argv) 
{
    /// NOTE: argv[1| = path?
    char* path = "kernel.config";

    if (kernel_init(path) != EXIT_SUCCESS) {
        fprintf(stderr, "[ERROR] inicializacion de kernel");
        return EXIT_FAILURE;
    }

    kernel_shutdown();

    return EXIT_SUCCESS;
}
