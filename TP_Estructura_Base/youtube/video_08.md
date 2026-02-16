# Video 8 - FIFO y Round Robin

**Duración estimada:** 15-20 minutos  
**Bloque:** Algoritmos de Planificación  

---

## Conceptos a Explicar

### FIFO (First-In-First-Out)
- También llamado FCFS (First-Come-First-Served)
- El más simple: orden de llegada
- No preemptivo: ejecuta hasta terminar o bloquearse
- **Problema:** Convoy effect (procesos cortos esperan detrás de largos)

### Round Robin (RR)
- FIFO + quantum (tiempo compartido)
- Preemptivo: desaloja tras quantum
- Fairness: todos los procesos progresan
- **Trade-off:** quantum muy corto → alto overhead de context switch

### Comparación
| Aspecto | FIFO | Round Robin |
|---------|------|-------------|
| Preemption | No | Sí |
| Quantum | N/A | Configurable |
| Fairness | Baja | Alta |
| Overhead | Bajo | Medio |
| Latencia | Variable | Predecible |

---

## Código y Demostración

### 1. FIFO
**Archivo:** `kernel/src/planificacion/algoritmo.c`

```c
t_pcb* algoritmo_obtener_fifo(void) {
    pthread_mutex_lock(&mutex_cola_ready);
    
    if (queue_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_cola_ready);
        return NULL;
    }
    
    // Simplemente sacar el primero
    t_pcb* pcb = queue_pop(cola_ready);
    
    pthread_mutex_unlock(&mutex_cola_ready);
    
    log_info(logger, "FIFO: Seleccionado PID=%d", pcb->pid);
    return pcb;
}
```

**Características:**
- Sin lógica de selección
- O(1) complejidad
- No necesita quantum

### 2. Round Robin
```c
t_pcb* algoritmo_obtener_rr(void) {
    // Exactamente igual que FIFO en la selección
    pthread_mutex_lock(&mutex_cola_ready);
    
    if (queue_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_cola_ready);
        return NULL;
    }
    
    t_pcb* pcb = queue_pop(cola_ready);
    
    pthread_mutex_unlock(&mutex_cola_ready);
    
    log_info(logger, "RR: Seleccionado PID=%d, quantum=%d ms", 
        pcb->pid, pcb->quantum);
    
    return pcb;
}
```

**La diferencia está en:**
1. El timer de quantum (en `corto_plazo.c`)
2. Cuando vuelve por quantum, va al final de la cola

---

## Demo: Test 5 (FIFO sin desalojo)

### Configuración
```ini
ALGORITMO_PLANIFICACION=FIFO
```

### Archivos de procesos
**proceso_largo.txt:**
```
SET AX 0
SET BX 1
SUM AX BX
SUM AX BX
... (100 instrucciones)
EXIT
```

**proceso_corto.txt:**
```
SET CX 5
EXIT
```

### Ejecución
```bash
so> RUN proceso_largo.txt
so> RUN proceso_corto.txt
so> START
```

### Logs esperados
```
[KERNEL] Despachando PID=1 (largo) a CPU
[CPU] Ejecutando instrucciones PID=1...
... (tarda 5 segundos)
[KERNEL] PID=1 finalizó
[KERNEL] Despachando PID=2 (corto) a CPU
[CPU] Ejecutando SET CX 5
[CPU] Ejecutando EXIT
[KERNEL] PID=2 finalizó (ejecutó rápido pero esperó mucho)
```

**Problema:** Proceso corto esperó innecesariamente.

---

## Demo: Test 4 (RR con desalojo)

### Configuración
```ini
ALGORITMO_PLANIFICACION=RR
QUANTUM=50
```

### Mismos archivos de procesos

### Ejecución
```bash
so> RUN proceso_largo.txt
so> RUN proceso_corto.txt
so> START
```

### Logs esperados
```
[KERNEL] Despachando PID=1 a CPU
[KERNEL] Timer: quantum=50 ms
... (después de 50ms)
[KERNEL] Quantum expirado: PID=1
[KERNEL] PID=1 → READY (al final de cola)
[KERNEL] Despachando PID=2 a CPU
[CPU] Ejecutando SET CX 5
[CPU] Ejecutando EXIT
[KERNEL] PID=2 finalizó (terminó rápido!)
[KERNEL] Despachando PID=1 a CPU (continúa donde quedó)
... (más ciclos RR hasta terminar)
```

**Mejora:** Proceso corto no esperó todo el tiempo.

---

## Análisis de Métricas

### Waiting Time (Tiempo de espera)
```
FIFO:
- PID=1: 0 ms (ejecuta primero)
- PID=2: 5000 ms (espera todo el proceso largo)
- Promedio: 2500 ms

RR (quantum=50ms):
- PID=1: ~50 ms (pausas por desalojo)
- PID=2: ~50 ms (ejecuta casi enseguida)
- Promedio: ~50 ms
```

### Turnaround Time
```
Turnaround = Tiempo_espera + Tiempo_ejecución

FIFO:
- PID=2: 5000 (espera) + 10 (ejecución) = 5010 ms

RR:
- PID=2: 50 (espera) + 10 (ejecución) = 60 ms
```

### Context Switches
```
FIFO: 2 (uno por proceso)
RR: ~100+ (depende del quantum y duración)
```

---

## Pros y Contras

### FIFO
**Pros:**
- Simplicidad máxima
- Cero overhead de context switch innecesario
- Predecible para batch processing

**Contras:**
- Convoy effect
- Mala interactividad
- No fair para procesos cortos

### Round Robin
**Pros:**
- Fairness: todos progresan
- Buena interactividad
- Latencia acotada: `latencia_max = n_procesos * quantum`

**Contras:**
- Overhead por context switches
- Quantum difícil de ajustar:
  - Muy corto → alto overhead
  - Muy largo → degrada a FIFO

---

## Elección del Quantum

### Ejemplo práctico
```c
// Si context_switch tarda 1ms y quantum=10ms:
Overhead = 1/(10+1) = ~9%

// Si quantum=100ms:
Overhead = 1/(100+1) = ~1%
```

**Regla empírica:**
- Desktop interactivo: 10-20 ms
- Server batch: 100+ ms
- Real-time: < 1 ms

---

## Puntos Clave a Destacar

1. **FIFO = RR con quantum infinito**
2. **Quantum define el trade-off** entre fairness y overhead
3. **Interactividad vs throughput:** diferentes objetivos requieren diferentes algoritmos
4. **En el TP:** Cambio dinámico de algoritmo permite experimentar

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Algoritmos más sofisticados: **VRR, HRRN, Prioridades**
- Criteria de selección basados en heurísticas
- Prevención de starvation
