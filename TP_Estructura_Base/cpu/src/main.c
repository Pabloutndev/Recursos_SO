#include <stdio.h>
#include <stdlib.h>
#include <cpu.h>

#define CPU_CONFIG "cpu.config"

int main(int argc, char* argv[])
{   
    cpu_init(CPU_CONFIG);
    cpu_run();
    cpu_shutdown();

    return EXIT_SUCCESS;
}