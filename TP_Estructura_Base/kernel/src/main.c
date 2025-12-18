// main.c
#include <mod_kernel.h>
#include <config/kernel_config.h>
#include <signal.h>


int main(int argc, char** argv) 
{
    printf("Hola mundo");
    /*signal(SIGINT, terminar);
    
    const char* config_path = (argc>1)? argv[1] : "configs/kernel.config";

    if (kernel_init(config_path) != 0) {
        fprintf(stderr,"error iniciando kernel\n");
        return EXIT_FAILURE;
    }
    
    log_info(logger,"LOG KERNEL \n");

    planificacion_init();
    
    ///TODO:
    // hilo principal: recibir consolas / RUN / ADD / METRICS
    // implementá tu loop de consola/servicio que crea PCBs y llama ingresar_new(pcb)
    while (1) {
        sleep(10);
        // placeholder: en tu implementación escuchás sockets y creás PCBs
    }

    kernel_shutdown();*/
    return 0;
}
