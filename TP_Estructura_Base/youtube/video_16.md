# Video 16 - MMU y TLB en la CPU

**Duración estimada:** 15-20 minutos  
**Bloque:** Gestión de Memoria

---

## Conceptos a Explicar

### MMU (Memory Management Unit)
- Hardware que traduce direcciones
- En nuestro TP: módulo de software en CPU
- Intercepta todos los accesos a memoria

### TLB (Translation Lookaside Buffer)
- Cache de traducciones recientes
- Evita consultar tabla de páginas cada vez
- Hit ratio crítico para performance

### Algoritmos de reemplazo TLB
- **FIFO:** First-In-First-Out
- **LRU:** Least Recently Used

---

## Código y Demostración

### Estructura TLB
**Archivo:** `cpu/src/tlb/tlb.h`

```c
typedef struct {
    bool valida;
    uint32_t pid;
    uint32_t pagina;
    uint32_t frame;
    uint64_t timestamp;  // Para LRU
} t_entrada_tlb;

typedef struct {
    t_entrada_tlb* entradas;
    uint32_t cantidad;
    algoritmo_tlb algoritmo;
    int indice_fifo;  // Para FIFO
    pthread_mutex_t mutex;
} t_tlb;
```

### Inicialización
```c
t_tlb* tlb_crear(uint32_t cantidad, char* algoritmo_str) {
    t_tlb* tlb = malloc(sizeof(t_tlb));
    tlb->cantidad = cantidad;
    tlb->entradas = calloc(cantidad, sizeof(t_entrada_tlb));
    tlb->indice_fifo = 0;
    
    if (strcmp(algoritmo_str, "FIFO") == 0) {
        tlb->algoritmo = ALG_FIFO;
    } else {
        tlb->algoritmo = ALG_LRU;
    }
    
    pthread_mutex_init(&tlb->mutex, NULL);
    
    log_info(logger, "TLB creada: %d entradas, algoritmo=%s", 
        cantidad, algoritmo_str);
    
    return tlb;
}
```

### Buscar en TLB
```c
bool tlb_buscar(t_tlb* tlb, uint32_t pid, uint32_t pagina, uint32_t* frame_out) {
    pthread_mutex_lock(&tlb->mutex);
    
    for (int i = 0; i < tlb->cantidad; i++) {
        if (tlb->entradas[i].valida &&
            tlb->entradas[i].pid == pid &&
            tlb->entradas[i].pagina == pagina) {
            
            // TLB HIT
            *frame_out = tlb->entradas[i].frame;
            
            // Actualizar timestamp (para LRU)
            tlb->entradas[i].timestamp = timestamp_actual();
            
            pthread_mutex_unlock(&tlb->mutex);
            
            log_trace(logger, "TLB HIT: PID=%d, pág=%d → frame=%d", 
                pid, pagina, *frame_out);
            
            return true;
        }
    }
    
    pthread_mutex_unlock(&tlb->mutex);
    
    log_trace(logger, "TLB MISS: PID=%d, pág=%d", pid, pagina);
    
    return false;
}
```

### Agregar a TLB (FIFO)
```c
void tlb_agregar_fifo(t_tlb* tlb, uint32_t pid, uint32_t pagina, uint32_t frame) {
    int indice = tlb->indice_fifo;
    
    tlb->entradas[indice].valida = true;
    tlb->entradas[indice].pid = pid;
    tlb->entradas[indice].pagina = pagina;
    tlb->entradas[indice].frame = frame;
    
    tlb->indice_fifo = (tlb->indice_fifo + 1) % tlb->cantidad;
    
    log_trace(logger, "TLB add (FIFO): índice=%d, PID=%d, pág=%d, frame=%d", 
        indice, pid, pagina, frame);
}
```

### Agregar a TLB (LRU)
```c
void tlb_agregar_lru(t_tlb* tlb, uint32_t pid, uint32_t pagina, uint32_t frame) {
    // Buscar entrada inválida o más antigua
    int indice_victima = 0;
    uint64_t min_timestamp = UINT64_MAX;
    
    for (int i = 0; i < tlb->cantidad; i++) {
        if (!tlb->entradas[i].valida) {
            indice_victima = i;
            break;
        }
        
        if (tlb->entradas[i].timestamp < min_timestamp) {
            min_timestamp = tlb->entradas[i].timestamp;
            indice_victima = i;
        }
    }
    
    tlb->entradas[indice_victima].valida = true;
    tlb->entradas[indice_victima].pid = pid;
    tlb->entradas[indice_victima].pagina = pagina;
    tlb->entradas[indice_victima].frame = frame;
    tlb->entradas[indice_victima].timestamp = timestamp_actual();
    
    log_trace(logger, "TLB add (LRU): índice=%d, PID=%d, pág=%d, frame=%d", 
        indice_victima, pid, pagina, frame);
}
```

### Limpiar TLB al cambiar proceso
```c
void tlb_limpiar_proceso(t_tlb* tlb, uint32_t pid) {
    pthread_mutex_lock(&tlb->mutex);
    
    int invalidadas = 0;
    
    for (int i = 0; i < tlb->cantidad; i++) {
        if (tlb->entradas[i].valida && tlb->entradas[i].pid == pid) {
            tlb->entradas[i].valida = false;
            invalidadas++;
        }
    }
    
    pthread_mutex_unlock(&tlb->mutex);
    
    log_info(logger, "TLB limpiada: PID=%d, %d entradas invalidadas", 
        pid, invalidadas);
}
```

---

## MMU: Traducción con TLB

**Archivo:** `cpu/src/mmu/mmu.c`

```c
bool mmu_traducir(uint32_t pid, uint32_t dir_logica, uint32_t* dir_fisica_out) {
    uint32_t pagina = dir_logica / config.TAM_PAGINA;
    uint32_t offset = dir_logica % config.TAM_PAGINA;
    uint32_t frame;
    
    // 1. Consultar TLB
    if (tlb_buscar(tlb, pid, pagina, &frame)) {
        // TLB HIT: no necesita ir a Memoria
        *dir_fisica_out = (frame * config.TAM_PAGINA) + offset;
        return true;
    }
    
    // 2. TLB MISS: pedir a Memoria
    log_trace(logger, "Consultando Memoria: PID=%d, pág=%d", pid, pagina);
    
    if (!solicitar_numero_frame(pid, pagina, &frame)) {
        log_error(logger, "Memoria devolvió error para PID=%d, pág=%d", 
            pid, pagina);
        return false;
    }
    
    // 3. Agregar a TLB
    if (config.ALGORITMO_TLB == ALG_FIFO) {
        tlb_agregar_fifo(tlb, pid, pagina, frame);
    } else {
        tlb_agregar_lru(tlb, pid, pagina, frame);
    }
    
    // 4. Calcular dirección física
    *dir_fisica_out = (frame * config.TAM_PAGINA) + offset;
    
    return true;
}
```

### Solicitar frame a Memoria
```c
bool solicitar_numero_frame(uint32_t pid, uint32_t pagina, uint32_t* frame_out) {
    t_paquete* paq = crear_paquete(MEMORIA_OBTENER_FRAME);
    escribir_uint32(paq, pid);
    escribir_uint32(paq, pagina);
    enviar_paquete(paq, socket_memoria);
    destruir_paquete(paq);
    
    op_code op = recibir_operacion(socket_memoria);
    
    if (op != MEMORIA_OK) {
        return false;
    }
    
    t_paquete* respuesta = recibir_paquete(socket_memoria);
    *frame_out = leer_uint32(respuesta);
    destruir_paquete(respuesta);
    
    return true;
}
```

---

## Flujo completo: MOV_IN con TLB

```
CPU                     MMU                 TLB                Memoria
 |                       |                   |                    |
 |-- MOV_IN AX 100 ----->|                   |                    |
 |                       |-- traducir(100) ->|                    |
 |                       |                   |-- buscar pág=0 --> |
 |                       |                   |<-- MISS ----------|
 |                       |-- obtener frame(0)  -----------------> |
 |                       |<-- frame=5 -----------------------------|
 |                       |-- agregar TLB ---->|                    |
 |                       |<-- dir_fis=1380 ---|                    |
 |-- read(1380) ------------------------------------------->       |
 |<-- datos=42 ---------------------------------------------|      |
 | AX=42                 |                   |                    |
```

---

## Demo: Logs con TLB

```
[CPU] MOV_IN AX 0
[MMU] Traduciendo: dir_log=0
[TLB] Buscando: PID=1, pág=0
[TLB] MISS
[MMU] Consultando Memoria: PID=1, pág=0
[MEMORIA] Devolviendo frame: pág=0 → frame=5
[TLB] Agregando (FIFO): índice=0, frame=5
[MMU] Dirección física: 1280
[MEMORIA] Read: dir_fis=1280 → 42
[CPU] AX=42

[CPU] MOV_IN BX 1
[MMU] Traduciendo: dir_log=1
[TLB] Buscando: PID=1, pág=0
[TLB] HIT! frame=5
[MMU] Dirección física: 1281 (sin consultar Memoria)
[MEMORIA] Read: dir_fis=1281 → 10
[CPU] BX=10
```

---

## Puntos Clave a Destacar

1. **Performance:** TLB evita latencia de red
2. **Hit ratio:** Típicamente 90%+ en programas reales
3. **Context switch:** Limpiar TLB al cambiar proceso
4. **Tamaño:** TLB pequeña (4-16 entradas típico)

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos **Reemplazo de páginas y Swap**
- Algoritmo Clock
- Page fault
- Bits dirty y use
