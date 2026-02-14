#include <consola.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    char* path = "consola.config";
    consola_init(path);
    consola_run();
    consola_shutdown();
    return EXIT_SUCCESS;
}
