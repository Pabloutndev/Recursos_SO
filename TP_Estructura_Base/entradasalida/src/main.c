
#include <stdlib.h>
#include <stdio.h>
#include "core/io_main.h"

int main(int argc, char** argv) {
    if(argc < 3) {
        printf("Uso: %s <NOMBRE_INTERFAZ> <PATH_CONFIG>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char* nombre = argv[1];
    char* path = argv[2];
    
    io_init(path, nombre);
    
    return EXIT_SUCCESS;
}
