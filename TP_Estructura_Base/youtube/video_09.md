# Video 9 - VRR, HRRN y Prioridades

**Duración estimada:** 15-20 minutos  
**Bloque:** Algoritmos de Planificación

---

## Conceptos a Explicar

### Virtual Round Robin (VRR)
- Extensión de RR que premia procesos con I/O
- Bonus de quantum para procesos "interactivos"
- Objetivo: mejorar tiempos de respuesta

### Highest Response Ratio Next (HRRN)
- Selecciona según ratio: `(tiempo_espera + tiempo_servicio) / tiempo_servicio`
- Combina SJF con aging
- Previene starvation

### Prioridades
- Cada proceso tiene prioridad asignada
- Menor número = mayor prioridad
- Riesgo: starvation de procesos baja prioridad

---

## Código y Demostración

### 1. Virtual Round Robin
**Archivo:** `kernel/src/planificacion/algoritmo.c`

```c
t_pcb* algoritmo_obtener_vrr(void) {
    pthread_mutex_lock(&mutex_cola_ready);
    
    if (queue_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_cola_ready);
        return NULL;
    }
    
    // Sacar el primero (como RR)
    t_pcb* pcb = queue_pop(cola_ready);
    
    // Si viene de IO, darle bonus de quantum
    if (pcb->vino_de_io) {
        pcb->quantum = config.QUANTUM + config.QUANTUM_BONUS;
        pcb->vino_de_io = false;
        log_info(logger, "VRR: PID=%d con bonus (q=%d)", 
            pcb->pid, pcb->quantum);
    } else {
        pcb->quantum = config.QUANTUM;
    }
    
    pthread_mutex_unlock(&mutex_cola_ready);
    
    return pcb;
}
```

**Cuándo marcar `vino_de_io`:**
```c
void manejar_io_completado(t_pcb* pcb) {
    pcb->vino_de_io = true;  // Marcar para VRR
    
    pthread_mutex_lock(&pcb->mutex);
    pcb->estado = ESTADO_READY;
    pthread_mutex_unlock(&pcb->mutex);
    
    pthread_mutex_lock(&mutex_cola_ready);
    queue_push(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_cola_ready);
    
    sem_post(&sem_hay_ready);
}
```

### 2. Highest Response Ratio Next
```c
t_pcb* algoritmo_obtener_hrrn(void) {
    pthread_mutex_lock(&mutex_cola_ready);
    
    if (queue_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_cola_ready);
        return NULL;
    }
    
    t_pcb* mejor = NULL;
    double mejor_ratio = 0.0;
    uint64_t tiempo_actual = timestamp_actual();
    
    // Iterar cola para encontrar mejor ratio
    t_list* temp_list = queue_to_list(cola_ready);
    
    LIST_ITERATE(temp_list, t_pcb*, pcb) {
        uint64_t espera = tiempo_actual - pcb->tiempo_llegada;
        uint64_t servicio = pcb->tiempo_uso_cpu + 1;  // +1 para evitar división por 0
        
        double ratio = (double)(espera + servicio) / servicio;
        
        log_trace(logger, "HRRN: PID=%d, ratio=%.2f (espera=%lu, servicio=%lu)", 
            pcb->pid, ratio, espera, servicio);
        
        if (ratio > mejor_ratio) {
            mejor_ratio = ratio;
            mejor = pcb;
        }
    }
    
    // Remover de la cola el seleccionado
    queue_remove(cola_ready, mejor);
    
    pthread_mutex_unlock(&mutex_cola_ready);
    
    log_info(logger, "HRRN: Seleccionado PID=%d, ratio=%.2f", 
        mejor->pid, mejor_ratio);
    
    return mejor;
}
```

**Actualizar tiempo de uso:**
```c
void actualizar_tiempo_uso_cpu(t_pcb* pcb, uint64_t milisegundos) {
    pcb->tiempo_uso_cpu += milisegundos;
}
```

### 3. Prioridades
```c
t_pcb* algoritmo_obtener_prioridad(void) {
    pthread_mutex_lock(&mutex_cola_ready);
    
    if (queue_is_empty(cola_ready)) {
        pthread_mutex_unlock(&mutex_cola_ready);
        return NULL;
    }
    
    t_pcb* mejor = NULL;
    uint32_t mejor_prioridad = UINT32_MAX;
    
    t_list* temp_list = queue_to_list(cola_ready);
    
    LIST_ITERATE(temp_list, t_pcb*, pcb) {
        if (pcb->prioridad < mejor_prioridad) {
            mejor_prioridad = pcb->prioridad;
            mejor = pcb;
        }
    }
    
    queue_remove(cola_ready, mejor);
    
    pthread_mutex_unlock(&mutex_cola_ready);
    
    log_info(logger, "PRIORIDAD: Seleccionado PID=%d, prioridad=%d", 
        mejor->pid, mejor->prioridad);
    
    return mejor;
}
```

### 4. Cambio dinámico de algoritmo
**Archivo:** `kernel/src/peticiones/planificacion.c`

```c
void set_algoritmo(char* nombre) {
    pthread_mutex_lock(&mutex_algoritmo);
    
    if (strcmp(nombre, "FIFO") == 0) {
        proximoAEjecutar = algoritmo_obtener_fifo;
    } else if (strcmp(nombre, "RR") == 0) {
        proximoAEjecutar = algoritmo_obtener_rr;
    } else if (strcmp(nombre, "VRR") == 0) {
        proximoAEjecutar = algoritmo_obtener_vrr;
    } else if (strcmp(nombre, "HRRN") == 0) {
        proximoAEjecutar = algoritmo_obtener_hrrn;
    } else if (strcmp(nombre, "PRIORIDAD") == 0) {
        proximoAEjecutar = algoritmo_obtener_prioridad;
    } else {
        log_error(logger, "Algoritmo desconocido: %s", nombre);
        pthread_mutex_unlock(&mutex_algoritmo);
        return;
    }
    
    algoritmo_actual = string_duplicate(nombre);
    log_info(logger, "Algoritmo cambiado a: %s", algoritmo_actual);
    
    pthread_mutex_unlock(&mutex_algoritmo);
}
```

---

## Demo: VRR con procesos I/O

### Configuración
```ini
ALGORITMO_PLANIFICACION=VRR
QUANTUM=100
QUANTUM_BONUS=50
```

### Procesos
**proceso_io.txt:**
```
SET AX 1
IO_GEN_SLEEP TECLADO 1000
SET BX 2
EXIT
```

**proceso_cpu.txt:**
```
SET CX 0
SUM CX CX
SUM CX CX
... (muchas sumas)
EXIT
```

### Logs esperados
```
[KERNEL] Despachando PID=1 (proceso_io), q=100ms
[KERNEL] PID=1 → BLOCKED (IO)
[KERNEL] Despachando PID=2 (proceso_cpu), q=100ms
[KERNEL] Quantum expirado: PID=2
[IO] Completado TECLADO para PID=1
[KERNEL] PID=1 → READY (marca vino_de_io=true)
[KERNEL] Despachando PID=1, q=150ms (BONUS!)
```

---

## Demo: Test 10 (Prioridades con PAUSE/START)

### Comandos
```bash
so> PAUSE
so> RUN proceso_alta_prioridad.txt 1
so> RUN proceso_media_prioridad.txt 5
so> RUN proceso_baja_prioridad.txt 10
so> ALGORITMO PRIORIDAD
so> START
```

### Orden de ejecución
```
1. PID=1 (prioridad 1)
2. PID=2 (prioridad 5)
3. PID=3 (prioridad 10)
```

---

## Comparación de Algoritmos

### Tabla resumen
| Algoritmo | Criterio | Preemptivo | Overhead | Fairness | Starvation |
|-----------|----------|------------|----------|----------|------------|
| FIFO | Orden llegada | No | Bajo | Baja | No |
| RR | Orden + quantum | Sí | Medio | Alta | No |
| VRR | RR + bonus I/O | Sí | Medio | Alta | No |
| HRRN | Response ratio | No | Medio | Alta | No |
| PRIORIDAD | Prioridad estática | Opcional | Bajo | Baja | Sí |

### Cuándo usar cada uno
- **FIFO:** Batch processing, cargas predecibles
- **RR:** Sistemas interactivos, time-sharing
- **VRR:** Desktop con mix CPU/IO
- **HRRN:** Balance entre SJF y fairness
- **PRIORIDAD:** Sistemas críticos, hard real-time

---

## Prevención de Starvation

### Problema
```c
// Con prioridades, esto puede nunca ejecutar:
PID=10, prioridad=99  // Siempre hay procesos más prioritarios
```

### Solución: Aging
```c
void aging_thread() {
    while (true) {
        sleep(AGING_INTERVAL);  // ej: cada 5 segundos
        
        pthread_mutex_lock(&mutex_cola_ready);
        
        LIST_ITERATE(cola_ready, t_pcb*, pcb) {
            if (pcb->prioridad > 0) {
                pcb->prioridad--;  // Aumenta prioridad con el tiempo
                log_trace(logger, "Aging: PID=%d, nueva prioridad=%d", 
                    pcb->pid, pcb->prioridad);
            }
        }
        
        pthread_mutex_unlock(&mutex_cola_ready);
    }
}
```

---

## Puntos Clave a Destacar

1. **VRR:** Reconoce el comportamiento pasado (I/O) para predecir futuro
2. **HRRN:** Aging automático en la fórmula
3. **Prioridades:** Poder, simplicidad, pero requiere cuidado con starvation
4. **Function pointers:** Cambiar algoritmo en runtime sin recompilar

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos el **cambio de contexto** en detalle
- Cómo se guarda y restaura el estado de un proceso
- Interacción Kernel ↔ CPU en el dispatch
