# Video 7 - Planificador de Corto Plazo

**Duración estimada:** 15-20 minutos  
**Bloque:** Gestión de Procesos

---

## Conceptos a Explicar

### Scheduling (Planificación)
- Decidir QUIÉN ejecuta y CUÁNDO
- Objetivos contradictorios:
  - Throughput (procesos/segundo)
  - Latencia (tiempo respuesta)
  - Fairness (justicia)

### Planificador de Corto Plazo (Short-Term Scheduler)
- Frecuencia: muy alta (milisegundos)
- Transición: READY → EXEC
- Implementa el algoritmo de scheduling

### Quantum
- Tiempo máximo de CPU antes de desalojo
- Solo en algoritmos preemptivos (RR, VRR)
- Típico: 10-100 ms

---

## Código y Demostración

### 1. Hilo principal del scheduler
**Archivo:** `kernel/src/planificacion/corto_plazo.c`

```c
void* hilo_planificador_corto_plazo(void* arg) {
    log_info(logger, "Planificador Corto Plazo iniciado");
    
    while (true) {
        // 1. Esperar que haya procesos listos
        sem_wait(&sem_hay_ready);
        
        // 2. Seleccionar próximo proceso (según algoritmo)
        t_pcb* pcb = proximoAEjecutar();
        
        if (!pcb) {
            // Cola vacía (race condition), reintentar
            continue;
        }
        
        // 3. Marcar como EXEC
        pthread_mutex_lock(&pcb->mutex);
        pcb->estado = ESTADO_EXEC;
        proceso_en_cpu = pcb->pid;
        pthread_mutex_unlock(&pcb->mutex);
        
        log_info(logger, "Transición: PID=%d, READY → EXEC", pcb->pid);
        
        // 4. Iniciar timer de quantum (si aplica)
        if (es_algoritmo_preemptivo()) {
            pthread_create(&thread_timer, NULL, timer_quantum, pcb);
        }
        
        // 5. Despachar a CPU (BLOQUEANTE)
        t_resultado resultado = enviar_proceso_a_cpu(pcb);
        
        // 6. Proceso volvió, atender resultado
        atender_resultado_cpu(pcb, resultado);
    }
}
```

### 2. Function pointer para algoritmo
```c
// Puntero a función: permite cambio dinámico
t_pcb* (*proximoAEjecutar)(void) = algoritmo_obtener_fifo;

void set_algoritmo(char* nombre) {
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
    }
    
    log_info(logger, "Algoritmo cambiado a: %s", nombre);
}
```

### 3. Timer de quantum
```c
void* timer_quantum(void* arg) {
    t_pcb* pcb = (t_pcb*) arg;
    uint32_t quantum_ms = pcb->quantum;
    
    log_info(logger, "Timer iniciado: PID=%d, quantum=%d ms", 
        pcb->pid, quantum_ms);
    
    // Dormir por el tiempo del quantum
    usleep(quantum_ms * 1000);  // usleep recibe microsegundos
    
    // Verificar si sigue en EXEC
    pthread_mutex_lock(&pcb->mutex);
    bool sigue_ejecutando = (pcb->estado == ESTADO_EXEC);
    pthread_mutex_unlock(&pcb->mutex);
    
    if (sigue_ejecutando) {
        log_info(logger, "Quantum expirado: PID=%d", pcb->pid);
        enviar_interrupcion_cpu(INTERRUPCION_QUANTUM);
    } else {
        log_info(logger, "Timer cancelado: PID=%d ya no está en EXEC", 
            pcb->pid);
    }
    
    return NULL;
}
```

### 4. Dispatch a CPU
**Archivo:** `kernel/src/peticiones/dispatch.c`

```c
t_resultado enviar_proceso_a_cpu(t_pcb* pcb) {
    log_info(logger, "Despachando PID=%d a CPU", pcb->pid);
    
    // 1. Serializar contexto
    t_paquete* paq = crear_paquete(CPU_DISPATCH);
    serializar_contexto(paq, pcb);
    enviar_paquete(paq, socket_cpu_dispatch);
    destruir_paquete(paq);
    
    // 2. Esperar resultado (BLOQUEANTE)
    op_code motivo = recibir_operacion(socket_cpu_dispatch);
    t_paquete* respuesta = recibir_paquete(socket_cpu_dispatch);
    
    // 3. Deserializar resultado
    t_resultado resultado;
    resultado.motivo = motivo;
    
    // Actualizar PC y registros
    pcb->pc = leer_uint32(respuesta);
    pcb->registros.AX = leer_uint8(respuesta);
    // ... resto de registros
    
    destruir_paquete(respuesta);
    
    log_info(logger, "CPU devolvió PID=%d, motivo=%s", 
        pcb->pid, motivo_a_string(motivo));
    
    return resultado;
}
```

### 5. Atender resultado de CPU
**Archivo:** `kernel/src/peticiones/interrupciones.c`

```c
void atender_resultado_cpu(t_pcb* pcb, t_resultado resultado) {
    // Marcar el proceso como no ejecutando
    proceso_en_cpu = 0;
    
    switch (resultado.motivo) {
        case MOTIVO_EXIT:
            manejar_exit(pcb);
            break;
        
        case MOTIVO_QUANTUM:
            manejar_quantum(pcb);
            break;
        
        case MOTIVO_IO:
            manejar_io(pcb, resultado.dispositivo, resultado.tiempo);
            break;
        
        case MOTIVO_WAIT:
            manejar_wait(pcb, resultado.recurso);
            break;
        
        case MOTIVO_SIGNAL:
            manejar_signal(pcb, resultado.recurso);
            break;
        
        case MOTIVO_SEGFAULT:
            manejar_segfault(pcb);
            break;
    }
}
```

**Archivo:** `kernel/src/peticiones/interrupciones_handlers.c`

```c
void manejar_quantum(t_pcb* pcb) {
    log_info(logger, "Proceso PID=%d desalojado por quantum", pcb->pid);
    
    // Volver a READY
    pthread_mutex_lock(&pcb->mutex);
    pcb->estado = ESTADO_READY;
    pthread_mutex_unlock(&pcb->mutex);
    
    // Encolar al final (Round Robin)
    pthread_mutex_lock(&mutex_cola_ready);
    queue_push(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_cola_ready);
    
    sem_post(&sem_hay_ready);
}

void manejar_exit(t_pcb* pcb) {
    log_info(logger, "Proceso PID=%d finalizó (EXIT)", pcb->pid);
    
    pthread_mutex_lock(&pcb->mutex);
    pcb->estado = ESTADO_EXIT;
    pthread_mutex_unlock(&pcb->mutex);
    
    liberar_recursos_proceso(pcb);
    solicitar_finalizacion_memoria(pcb->pid);
    pcb_destruir(pcb);
    
    // Liberar slot de multiprogramación
    sem_post(&sem_mp);
}
```

---

## Demo: Test 4 (Quantum RR)

### Configuración
```ini
ALGORITMO_PLANIFICACION=RR
QUANTUM=50
```

### Archivo: `test_rr.txt`
```
SET AX 1
SET BX 2
SUM AX BX
SUM AX BX
SUM AX BX
... (20 instrucciones)
EXIT
```

### Logs esperados (con quantum corto)
```
[KERNEL] Despachando PID=1 a CPU
[KERNEL] Timer iniciado: PID=1, quantum=50 ms
[CPU] Ejecutando SET AX 1
[CPU] Ejecutando SET BX 2
[KERNEL] Quantum expirado: PID=1
[CPU] Interrupción recibida: QUANTUM
[CPU] Devolviendo PID=1, PC=3
[KERNEL] CPU devolvió PID=1, motivo=QUANTUM
[KERNEL] Proceso PID=1 desalojado por quantum
[KERNEL] Transición: PID=1, EXEC → READY
... (proceso vuelve a encolar y ejecutar)
```

---

## Diagrama de secuencia

```
Scheduler CP ──→ seleccionar PCB
              │
              └─→ dispatch ──→ CPU
                               │
                               └─→ ejecutar instrucciones
                                   │
                                   ├─→ EXIT ──→ finalizar
                                   ├─→ QUANTUM ──→ READY
                                   └─→ IO ──→ BLOCKED
```

---

## Puntos Clave a Destacar

1. **Bloqueante:** El scheduler espera hasta que CPU devuelve el proceso
2. **Preemption:** Timer de quantum desaloja procesos largos
3. **Modularidad:** Function pointer para cambiar algoritmo en runtime
4. **Race conditions:** Semáforos y mutexes protegen colas

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos los **algoritmos de planificación** en detalle
- FIFO vs Round Robin
- Comparación de comportamiento
- Tests específicos
