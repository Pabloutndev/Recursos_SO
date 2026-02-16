# Video 15 - Paginación

**Duración estimada:** 20-25 minutos  
**Bloque:** Gestión de Memoria

---

## Conceptos a Explicar

### Paginación
- División del espacio de direcciones en páginas de tamaño fijo
- RAM dividida en frames (mismo tamaño que páginas)
- **Ventaja:** eliminan fragmentación externa
- **Desventaja:** fragmentación interna (última página)

### Tabla de páginas
- Mapeo: número_pagina → número_frame
- Una tabla por proceso
- Metad atos: presente, dirty, use

### Traducción de direcciones
```
Dirección lógica = Número_página | Offset
Número_página = dirección_lógica / tamaño_página
Offset = dirección_lógica % tamaño_página
Dirección física = (Frame * tamaño_página) + Offset
```

---

## Código y Demostración

### 1. Entrada de tabla de páginas
**Archivo:** `memoria/src/gestion/paginas_internal.h`

```c
typedef struct {
    uint32_t frame;     // Número de frame asignado
    bool presente;      // ¿Está en RAM o en swap?
    bool dirty;         // ¿Fue modificada?
    bool use;           // Bit de uso (para reemplazo Clock)
} t_entrada_pagina;
```

### 2. Tabla de páginas
**Archivo:** `memoria/src/gestion/paginas.c`

```c
typedef struct {
    uint32_t pid;
    t_list* entradas;  // Lista de t_entrada_pagina
    uint32_t cantidad_paginas;
} t_tabla_paginas;

t_tabla_paginas* crear_tabla_paginas(uint32_t pid) {
    t_tabla_paginas* tabla = malloc(sizeof(t_tabla_paginas));
    tabla->pid = pid;
    tabla->entradas = list_create();
    tabla->cantidad_paginas = 0;
    
    log_info(logger, "Tabla de páginas creada para PID=%d", pid);
    
    return tabla;
}
```

### 3. Agregar página
```c
bool agregar_pagina(t_tabla_paginas* tabla) {
    // Buscar frame libre
    int frame = bitmap_obtener_frame_libre();
    
    if (frame < 0) {
        log_warning(logger, "No hay frames libres, se requiere reemplazo");
        // Invocar algoritmo de reemplazo
        frame = reemplazo_obtener_victima();
    }
    
    // Crear entrada
    t_entrada_pagina* entrada = malloc(sizeof(t_entrada_pagina));
    entrada->frame = frame;
    entrada->presente = true;
    entrada->dirty = false;
    entrada->use = false;
    
    list_add(tabla->entradas, entrada);
    tabla->cantidad_paginas++;
    
    // Marcar frame como ocupado
    bitmap_ocupar_frame(frame);
    
    log_info(logger, "Página %d agregada: frame=%d", 
        tabla->cantidad_paginas - 1, frame);
    
    return true;
}
```

### 4. Traducción
```c
bool paginacion_traducir(uint32_t pid, uint32_t dir_logica, uint32_t* dir_fisica_out) {
    t_tabla_paginas* tabla = obtener_tabla(pid);
    
    // Calcular página y offset
    uint32_t num_pagina = dir_logica / config.TAM_PAGINA;
    uint32_t offset = dir_logica % config.TAM_PAGINA;
    
    log_trace(logger, "Traducción: PID=%d, dir_log=%d → página=%d, offset=%d", 
        pid, dir_logica, num_pagina, offset);
    
    // Verificar bounds
    if (num_pagina >= tabla->cantidad_paginas) {
        log_error(logger, "SEGFAULT: página %d fuera de rango (PID=%d)", 
            num_pagina, pid);
        return false;
    }
    
    // Obtener entrada
    t_entrada_pagina* entrada = list_get(tabla->entradas, num_pagina);
    
    // Verificar presencia
    if (!entrada->presente) {
        log_info(logger, "PAGE FAULT: página %d no presente (PID=%d)", 
            num_pagina, pid);
        // Traer de swap
        cargar_pagina_de_swap(tabla, num_pagina);
    }
    
    // Calcular dirección física
    uint32_t dir_fisica = (entrada->frame * config.TAM_PAGINA) + offset;
    
    // Actualizar metadatos
    entrada->use = true;  // Para algoritmo Clock
    
    *dir_fisica_out = dir_fisica;
    
    log_trace(logger, "Traducción exitosa: frame=%d, dir_fis=%d", 
        entrada->frame, dir_fisica);
    
    return true;
}
```

---

## Bitmap de Frames

**Archivo:** `memoria/src/gestion/frames.c`

```c
t_bitarray* bitmap_frames;
pthread_mutex_t mutex_bitmap;

void frames_inicializar(uint32_t cantidad) {
    size_t bytes = (cantidad + 7) / 8;  // Redondear
    void* bits = calloc(1, bytes);
    
    bitmap_frames = bitarray_create_with_mode(bits, bytes, LSB_FIRST);
    pthread_mutex_init(&mutex_bitmap, NULL);
    
    log_info(logger, "Bitmap inicializado: %d frames", cantidad);
}

int bitmap_obtener_frame_libre() {
    pthread_mutex_lock(&mutex_bitmap);
    
    for (int i = 0; i < config.CANTIDAD_FRAMES; i++) {
        if (!bitarray_test_bit(bitmap_frames, i)) {
            pthread_mutex_unlock(&mutex_bitmap);
            return i;
        }
    }
    
    pthread_mutex_unlock(&mutex_bitmap);
    return -1;  // No hay libres
}

void bitmap_ocupar_frame(int frame) {
    pthread_mutex_lock(&mutex_bitmap);
    bitarray_set_bit(bitmap_frames, frame);
    pthread_mutex_unlock(&mutex_bitmap);
}

void bitmap_liberar_frame(int frame) {
    pthread_mutex_lock(&mutex_bitmap);
    bitarray_clean_bit(bitmap_frames, frame);
    pthread_mutex_unlock(&mutex_bitmap);
}
```

---

## Resize de proceso

```c
bool paginacion_resize(uint32_t pid, uint32_t nuevo_tamanio) {
    t_tabla_paginas* tabla = obtener_tabla(pid);
    
    uint32_t paginas_necesarias = (nuevo_tamanio + config.TAM_PAGINA - 1) / config.TAM_PAGINA;
    uint32_t paginas_actuales = tabla->cantidad_paginas;
    
    if (paginas_necesarias > paginas_actuales) {
        // Crecer: agregar páginas
        for (int i = 0; i < paginas_necesarias - paginas_actuales; i++) {
            if (!agregar_pagina(tabla)) {
                log_error(logger, "No se pudo agregar página para PID=%d", pid);
                return false;
            }
        }
    } else if (paginas_necesarias < paginas_actuales) {
        // Achicar: liberar páginas
        for (int i = paginas_actuales - 1; i >= (int)paginas_necesarias; i--) {
            t_entrada_pagina* entrada = list_remove(tabla->entradas, i);
            bitmap_liberar_frame(entrada->frame);
            free(entrada);
            tabla->cantidad_paginas--;
        }
    }
    
    log_info(logger, "Resize: PID=%d, páginas=%d→%d", pid, paginas_actuales, tabla->cantidad_paginas);
    
    return true;
}
```

---

## Demo: Test 6 (MOV_OUT + MOV_IN)

### Archivo de prueba
```
SET AX 42
MOV_OUT 0 AX
SET AX 0
MOV_IN AX 0
EXIT
```

### Logs esperados
```
[CPU] SET AX 42 → AX=42
[CPU] MOV_OUT 0 AX → Solicita write a Memoria
[MEMORIA] Write: PID=1, dir_log=0, tamanio=1
[MEMORIA] Traducción: página=0, offset=0 → frame=5, dir_fis=1280
[MEMORIA] RAM[1280] = 42
[CPU] SET AX 0 → AX=0
[CPU] MOV_IN AX 0 → Solicita read a Memoria
[MEMORIA] Read: PID=1, dir_log=0, tamanio=1
[MEMORIA] Traducción: página=0, offset=0 → frame=5, dir_fis=1280
[MEMORIA] Datos leídos: 42
[CPU] AX = 42
[CPU] EXIT
```

---

## Puntos Clave a Destacar

1. **Tamaño fijo:** Simplifica gestión
2. **Fragmentación interna:** Desperdicio en última página
3. **Tabla por proceso:** Aislamiento completo
4. **Bits de control:** Presente, dirty, use son críticos

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos **MMU y TLB** en la CPU
- Cache de traducciones
- Algoritmos FIFO y LRU
- Aceleración de accesos a memoria
