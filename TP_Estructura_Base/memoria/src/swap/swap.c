#include "swap.h"
#include "../mod_memoria.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

static int tam_pagina = 0;

int swap_init(void) {
    tam_pagina = MEMORIA_CTX.config->tam_pagina;
    // Check if swap dir exists, create if not
    struct stat st = {0};
    if (stat("swap_files", &st) == -1) {
        mkdir("swap_files", 0700);
    }
    return 0;
}

static char* get_swap_path(uint32_t pid) {
    char* path = malloc(50);
    snprintf(path, 50, "swap_files/%u.swap", pid);
    return path;
}

bool swap_escribir_pagina(uint32_t pid, int nro_pagina, void* contenido) {
    char* path = get_swap_path(pid);
    //int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    free(path);
    
    if (fd < 0) return false;

    // Seek a la posicion de pagina
    off_t offset = nro_pagina * tam_pagina;

    // Ensure file size
    ftruncate(fd, offset + tam_pagina);

    lseek(fd, offset, SEEK_SET);
    if (write(fd, contenido, tam_pagina) != tam_pagina) {
        close(fd);
        return false;
    }
    close(fd);
    
    log_debug(MEMORIA_CTX.logger, "PID: %u - Swap out pagina %d", pid, nro_pagina);
    return true;
}

bool swap_leer_pagina(uint32_t pid, int nro_pagina, void* buffer) {
    char* path = get_swap_path(pid);
    int fd = open(path, O_RDONLY);
    free(path);

    if (fd < 0) return false;

    off_t offset = nro_pagina * tam_pagina;
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
        close(fd);
        return false;
    }
    ssize_t leidos = read(fd, buffer, tam_pagina);
    close(fd);

    if (leidos != tam_pagina) {
        return false;
    }

    log_debug(MEMORIA_CTX.logger, "PID: %u - Swap in pagina %d", pid, nro_pagina);
    return true;
}

void swap_borrar_proceso(uint32_t pid) {
    char* path = get_swap_path(pid);
    unlink(path);
    free(path);
}
