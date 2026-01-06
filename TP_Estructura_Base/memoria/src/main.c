#include <stdlib.h>
#include "mod_memoria.h"

int main(int argc, char** argv) {
    char* config_path = "memoria.config";
    if (argc > 1) {
        config_path = argv[1];
    }

    if (memoria_init(config_path) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    memoria_run();

    memoria_shutdown();
    return EXIT_SUCCESS;
}