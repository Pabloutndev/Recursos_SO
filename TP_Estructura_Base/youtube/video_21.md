# Video 21 - Detección de Deadlock: Algoritmo del Banquero

**Duración estimada:** 15-20 minutos  
**Bloque:** Sincronización

---

## Conceptos

### Estado Seguro vs Inseguro
- **Seguro:** Existe secuencia donde todos terminan
- **Inseguro:** Posible deadlock

### Matrices del Banquero
- **Available:** instancias libres de cada recurso
- **Allocation:** recursos asignados a cada proceso
- **Need:** recursos que aún necesita cada proceso

### Simulación
- Buscar proceso que pueda terminar (Need <= Available)
- Liberar sus recursos
- Repetir hasta que todos terminen o ninguno pueda

---

## Código

### Recopilar datos
```c
typedef struct {
    int* available;          // [cantidad_recursos]
    int** allocation;        // [procesos][recursos]
    int** need;              // [procesos][recursos]
    int cantidad_procesos;
    int cantidad_recursos;
} t_estado_banquero;

t_estado_banquero* banquero_recopilar_estado() {
    t_estado_banquero* estado = malloc(sizeof(t_estado_banquero));
    
    estado->cantidad_recursos = dictionary_size(recursos);
    estado->cantidad_procesos = dictionary_size(procesos_pcb);
    
    // Available
    estado->available = calloc(estado->cantidad_recursos, sizeof(int));
    int i = 0;
    DICTIONARY_ITERATE(recursos, char*, nombre, t_recurso*, rec) {
        estado->available[i++] = rec->instancias;
    }
    
    // Allocation y Need
    estado->allocation = malloc(estado->cantidad_procesos * sizeof(int*));
    estado->need = malloc(estado->cantidad_procesos * sizeof(int*));
    
    // ... llenar matrices desde PCBs
    
    return estado;
}
```

### Algoritmo
```c
bool banquero_es_estado_seguro(t_estado_banquero* estado) {
    bool* terminado = calloc(estado->cantidad_procesos, sizeof(bool));
    int* work = malloc(estado->cantidad_recursos * sizeof(int));
    memcpy(work, estado->available, estado->cantidad_recursos * sizeof(int));
    
    int terminados = 0;
    
    while (terminados < estado->cantidad_procesos) {
        bool found = false;
        
        for (int i = 0; i < estado->cantidad_procesos; i++) {
            if (terminado[i]) continue;
            
            // ¿Need[i] <= Work?
            bool puede_terminar = true;
            for (int j = 0; j < estado->cantidad_recursos; j++) {
                if (estado->need[i][j] > work[j]) {
                    puede_terminar = false;
                    break;
                }
            }
            
            if (puede_terminar) {
                // Simular terminación
                for (int j = 0; j < estado->cantidad_recursos; j++) {
                    work[j] += estado->allocation[i][j];
                }
                
                terminado[i] = true;
                terminados++;
                found = true;
                
                log_trace(logger, "Banquero: proceso %d puede terminar", i);
            }
        }
        
        if (!found) {
            // Ningún proceso pudo avanzar → INSEGURO
            free(terminado);
            free(work);
            return false;
        }
    }
    
    free(terminado);
    free(work);
    return true;  // SEGURO
}
```

---

## Comparación

| Aspecto | Grafo | Banquero |
|---------|-------|----------|
| Detecta | Deadlock actual | Estado inseguro |
| Cuándo | Después del hecho | Preventivo |
| Overhead | Bajo | Alto |
| Usado en | Sistemas reales | Académico |

---

## Las 4 Condiciones de Coffman

1. **Exclusión mutua:** Solo 1 proceso usa el recurso
2. **Hold and wait:** Proceso tiene recursos y pide más
3. **No preemption:** Recursos no se quitan forzosamente
4. **Espera circular:** Ciclo en el grafo

**Para prevenir deadlock:** Romper al menos una condición.

---

## Puntos Clave

1. **Preventivo:** Banquero evita deadlock
2. **Overhead:** Matriz requiere conocer necesidades futuras
3. **Real:** Linux usa grafo, no banquero

---

## Siguiente Video

Veremos **I/O Genérica, STDIN y STDOUT**.
