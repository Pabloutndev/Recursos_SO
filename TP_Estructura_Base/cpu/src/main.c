#include <stdio.h>
#include <stdlib.h>
#include <cpu.h>

#define PATH_CPU_CONFIG "cpu.config"

int main(int argc, char* argv[])
{   
    cpu_init(PATH_CPU_CONFIG);
    cpu_run();
    cpu_shutdown();

    return EXIT_SUCCESS;
}