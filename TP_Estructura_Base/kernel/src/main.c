// main.c
//#include <loggers/logger.h>

//#include <config/kernel_config.h>
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

    if(kernel_init(path) != EXIT_SUCCESS) {
        fprintf(stderr, "[ERROR] inicializacion de kernel");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

    ///TODO: DELETE?
    //printf("Hola mundo");
    
    /// TODO:
    //conexiones_init();

    //planificacion_init();

    //iniciar_consola();

    //kernel_shutdown();

    return 0;
}
