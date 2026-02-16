# Video 17 - Reemplazo de Páginas y Swap

**Duración estimada:** 15-20 minutos  
**Bloque:** Gestión de Memoria

---

## Conceptos

### Page Fault
- Acceso a página no presente en RAM
- Debe traerse desde swap (disco)
- Si no hay frames libres → reemplazo

### Algoritmo Clock
- Aproximación de LRU
- Puntero circular + bit de uso
- Segunda oportunidad antes de reemplazar

### Swap
- Archivo en disco como extensión de RAM
- Almacena páginas desalojadas
- Más lento que RAM

---

## Código

### Algoritmo Clock
**Archivo:** `memoria/src/gestion/reemplazo.c`

```c
static int puntero_clock = 0;

int reemplazo_clock() {
    while (true) {
        // Obtener todas las entradas de todas las tablas
        t_entrada_pagina* entrada = obtener_entrada_frame(puntero_clock);
        
        if (!entrada) {
            // Frame libre
            int victima = puntero_clock;
            puntero_clock = (puntero_clock + 1) % config.CANTIDAD_FRAMES;
            return victima;
        }
        
        if (entrada->use == false) {
            // Víctima encontrada
            int victima = puntero_clock;
            puntero_clock = (puntero_clock + 1) % config.CANTIDAD_FRAMES;
            
            log_info(logger, "Clock: víctima frame=%d", victima);
            
            // Si dirty, escribir a swap
            if (entrada->dirty) {
                swap_escribir_pagina(entrada);
            }
            
            return victima;
        }
        
        // Segunda oportunidad
        entrada->use = false;
        puntero_clock = (puntero_clock + 1) % config.CANTIDAD_FRAMES;
    }
}
```

### Swap: Escribir
**Archivo:** `memoria/src/swap/swap.c`

```c
void swap_escribir_pagina(uint32_t pid, uint32_t pagina, void* datos) {
    char* filename = string_from_format("%s/%d.swap", config.PATH_SWAP, pid);
    
    FILE* archivo = fopen(filename, "r+b");
    if (!archivo) {
        archivo = fopen(filename, "w+b");
    }
    
    fseek(archivo, pagina * config.TAM_PAGINA, SEEK_SET);
    fwrite(datos, config.TAM_PAGINA, 1, archivo);
    
    fclose(archivo);
    free(filename);
    
    log_info(logger, "Swap write: PID=%d, pág=%d", pid, pagina);
}
```

### Swap: Leer
```c
void swap_leer_pagina(uint32_t pid, uint32_t pagina, void* destino) {
    char* filename = string_from_format("%s/%d.swap", config.PATH_SWAP, pid);
    
    FILE* archivo = fopen(filename, "rb");
    
    fseek(archivo, pagina * config.TAM_PAGINA, SEEK_SET);
    fread(destino, config.TAM_PAGINA, 1, archivo);
    
    fclose(archivo);
    free(filename);
    
    log_info(logger, "Swap read: PID=%d, pág=%d", pid, pagina);
}
```

---

## Puntos Clave

1. **Clock:** Eficiente y justo
2. **Dirty bit:** Evita escrituras innecesarias a swap
3. **Swap:** Permite memoria virtual > RAM física

---

## Siguiente Video

Veremos **Segmentación** como alternativa a paginación.
