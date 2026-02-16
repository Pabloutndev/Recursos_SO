# Video 18 - Segmentación

**Duración estimada:** 15 minutos  
**Bloque:** Segmentación

---

## Conceptos

### Segmentación
- Partición contigua de memoria
- Un segmento por proceso
- Base + límite

### Fragmentación Externa
- Huecos entre segmentos
- Compactación necesaria

### Comparación con Paginación
- Paginación: tamaño fijo, sin fragmentación externa
- Segmentación: tamaño variable, más simple

---

## Código

### Estructura
```c
typedef struct {
    uint32_t pid;
    uint32_t base;     // Inicio en RAM
    uint32_t limite;   // Tamaño
} t_segmento;
```

### Traducción
```c
bool segmentacion_traducir(uint32_t pid, uint32_t offset, uint32_t* dir_fis) {
    t_segmento* seg = obtener_segmento(pid);
    
    if (offset >= seg->limite) {
        log_error(logger, "SEGFAULT: offset=%d >= limite=%d", offset, seg->limite);
        return false;
    }
    
    *dir_fis = seg->base + offset;
    return true;
}
```

### Free List (First-Fit)
```c
typedef struct {
    uint32_t base;
    uint32_t tamanio;
} t_hueco;

t_list* huecos_libres;

t_hueco* encontrar_hueco(uint32_t tamanio) {
    LIST_ITERATE(huecos_libres, t_hueco*, hueco) {
        if (hueco->tamanio >= tamanio) {
            return hueco;
        }
    }
    return NULL;
}
```

---

## Puntos Clave

1. **Simplicidad:** No hay tabla de páginas
2. **Fragmentación externa:** Problema principal
3. **Resize:** Puede requerir reubicación

---

## Siguiente Video

Veremos **Recursos y sincronización** (WAIT/SIGNAL).
