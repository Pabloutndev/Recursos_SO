# Video 14 - Memoria: Conceptos y Arquitectura

**Duración estimada:** 10-15 minutos  
**Bloque:** Gestión de Memoria

---

## Conceptos a Explicar

### Memoria Virtual
- Abstracción: cada proceso ve su propio espacio
- Aislamiento: procesos no se leen/escriben entre sí
- Flexibilidad: tamaño mayor que RAM física

### Espacio de direcciones
- **Lógico:** visible al proceso (0...N)
- **Físico:** RAM real
- **Traducción:** lógico → físico (por hardware+SO)

### ¿Por qué abstraer la memoria?
- Simplifica programación
- Protección entre procesos
- Permite swap (disco como extensión RAM)

---

## Código y Demostración

### 1. Estructura del módulo Memoria
**Archivo:** `memoria/src/gestion/memoria_core.c`

```c
// Estado global del módulo
typedef struct {
    void* ram;  // Array de bytes
    uint32_t tamanio_ram;
    uint32_t tamanio_pagina;
    
    t_dictionary* procesos;  // PID → t_proceso_memoria
    
    t_bitmap* bitmap_frames;
    pthread_mutex_t mutex_ram;
} t_memoria_estado;

t_memoria_estado* memoria;
```

### 2. Crear proceso en Memoria
```c
bool memoria_crear_proceso(uint32_t pid, char* path) {
    log_info(logger, "Creando proceso PID=%d", pid);
    
    // 1. Abrir archivo de instrucciones
    FILE* archivo = fopen(path, "r");
    if (!archivo) return false;
    
    // 2. Leer instrucciones
    t_list* instrucciones = list_create();
    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) {
        linea[strcspn(linea, "\n")] = 0;
        list_add(instrucciones, string_duplicate(linea));
    }
    fclose(archivo);
    
    // 3. Crear estructura
    t_proceso_memoria* proceso = malloc(sizeof(t_proceso_memoria));
    proceso->pid = pid;
    proceso->instrucciones = instrucciones;
    proceso->tabla_paginas = crear_tabla_paginas();
    
    // 4. Guardar
    dictionary_put(memoria->procesos, string_itoa(pid), proceso);
    
    return true;
}
```

### 3. Fetch instrucción
```c
char* memoria_fetch_instruccion(uint32_t pid, uint32_t pc) {
    t_proceso_memoria* proceso = obtener_proceso(pid);
    
    if (!proceso) {
        log_error(logger, "Proceso PID=%d no existe", pid);
        return NULL;
    }
    
    if (pc >= list_size(proceso->instrucciones)) {
        log_error(logger, "PC=%d fuera de rango para PID=%d", pc, pid);
        return NULL;
    }
    
    char* instruccion = list_get(proceso->instrucciones, pc);
    
    log_trace(logger, "Fetch: PID=%d, PC=%d → '%s'", pid, pc, instruccion);
    
    return string_duplicate(instruccion);
}
```

---

## RAM: Array de bytes

### 1. Inicialización
**Archivo:** `memoria/src/gestion/memoria_ram.c`

```c
void* ram_inicializar(uint32_t tamanio) {
    void* ram = malloc(tamanio);
    memset(ram, 0, tamanio);
    
    log_info(logger, "RAM inicializada: %d bytes", tamanio);
    
    return ram;
}
```

### 2. Leer memoria física
```c
uint8_t ram_leer_byte(void* ram, uint32_t direccion_fisica) {
    uint8_t* ptr = (uint8_t*) ram;
    return ptr[direccion_fisica];
}

void ram_leer_bloque(void* ram, uint32_t dir_fisica, void* destino, uint32_t tamanio) {
    memcpy(destino, ram + dir_fisica, tamanio);
    log_trace(logger, "RAM read: dir=%d, tamanio=%d", dir_fisica, tamanio);
}
```

### 3. Escribir memory física
```c
void ram_escribir_byte(void* ram, uint32_t direccion_fisica, uint8_t valor) {
    uint8_t* ptr = (uint8_t*) ram;
    ptr[direccion_fisica] = valor;
}

void ram_escribir_bloque(void* ram, uint32_t dir_fisica, void* origen, uint32_t tamanio) {
    memcpy(ram + dir_fisica, origen, tamanio);
    log_trace(logger, "RAM write: dir=%d, tamanio=%d", dir_fisica, tamanio);
}
```

---

## Esquema de Memoria: Capa de abstracción

**Archivo:** `memoria/src/gestion/esquema_memoria.c`

```c
typedef enum {
    ESQUEMA_PAGINACION,
    ESQUEMA_SEGMENTACION
} tipo_esquema;

typedef struct {
    tipo_esquema tipo;
    
    // Function pointers
    bool (*traducir)(uint32_t pid, uint32_t dir_logica, uint32_t* dir_fisica);
    bool (*crear_proceso)(uint32_t pid);
    void (*destruir_proceso)(uint32_t pid);
    bool (*resize)(uint32_t pid, uint32_t nuevo_tamanio);
} t_esquema_memoria;

t_esquema_memoria* esquema;
```

### Inicialización
```c
void esquema_inicializar(char* tipo_str) {
    esquema = malloc(sizeof(t_esquema_memoria));
    
    if (strcmp(tipo_str, "PAGINACION") == 0) {
        esquema->tipo = ESQUEMA_PAGINACION;
        esquema->traducir = paginacion_traducir;
        esquema->crear_proceso = paginacion_crear_proceso;
        esquema->destruir_proceso = paginacion_destruir_proceso;
        esquema->resize = paginacion_resize;
    }
    else if (strcmp(tipo_str, "SEGMENTACION") == 0) {
        esquema->tipo = ESQUEMA_SEGMENTACION;
        esquema->traducir = segmentacion_traducir;
        // ...
    }
    
    log_info(logger, "Esquema de memoria: %s", tipo_str);
}
```

---

## Handlers de requests

**Archivo:** `memoria/src/adaptadores/memoria_adapter.c`

```c
void handle_crear_proceso(int socket) {
    t_paquete* paq = recibir_paquete(socket);
    uint32_t pid = leer_uint32(paq);
    char* path = leer_string(paq);
    
    bool ok = memoria_crear_proceso(pid, path);
    
    if (ok) {
        enviar_operacion(MEMORIA_OK, socket);
    } else {
        enviar_operacion(MEMORIA_ERROR, socket);
    }
    
    free(path);
    destruir_paquete(paq);
}

void handle_fetch_instruccion(int socket) {
    t_paquete* paq = recibir_paquete(socket);
    uint32_t pid = leer_uint32(paq);
    uint32_t pc = leer_uint32(paq);
    
    char* instruccion = memoria_fetch_instruccion(pid, pc);
    
    if (instruccion) {
        t_paquete* respuesta = crear_paquete(MEMORIA_OK);
        escribir_string(respuesta, instruccion);
        enviar_paquete(respuesta, socket);
        destruir_paquete(respuesta);
        free(instruccion);
    } else {
        enviar_operacion(MEMORIA_ERROR, socket);
    }
    
    destruir_paquete(paq);
}

void handle_read(int socket) {
    t_paquete* paq = recibir_paquete(socket);
    uint32_t pid = leer_uint32(paq);
    uint32_t dir_logica = leer_uint32(paq);
    uint32_t tamanio = leer_uint32(paq);
    
    // Traducir dirección
    uint32_t dir_fisica;
    if (!esquema->traducir(pid, dir_logica, &dir_fisica)) {
        enviar_operacion(MEMORIA_SEGFAULT, socket);
        destruir_paquete(paq);
        return;
    }
    
    // Leer RAM
    void* datos = malloc(tamanio);
    ram_leer_bloque(memoria->ram, dir_fisica, datos, tamanio);
    
    // Enviar respuesta
    t_paquete* respuesta = crear_paquete(MEMORIA_OK);
    escribir_bloque(respuesta, datos, tamanio);
    enviar_paquete(respuesta, socket);
    
    free(datos);
    destruir_paquete(respuesta);
    destruir_paquete(paq);
}
```

---

## Puntos Clave a Destacar

1. **Abstracción:** Memoria es un servicio para otros módulos
2. **Dos tipos de datos:** Instrucciones (fetch) vs Datos (read/write)
3. **Esquema configurable:** Paginación o segmentación
4. **Thread-safety:** Mutex protege accesos a RAM

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos **Paginación** en detalle
- Páginas y frames
- Tabla de páginas
- Traducción de direcciones
- Demo con MOV_IN/MOV_OUT
