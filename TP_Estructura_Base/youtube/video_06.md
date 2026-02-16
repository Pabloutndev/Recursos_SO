# Video 6 - Planificador de Largo Plazo

**Duración estimada:** 10-15 minutos  
**Bloque:** Gestión de Procesos

---

## Conceptos a Explicar

### Grado de Multiprogramación
- Límite de procesos simultáneos en memoria
- Control de admisión: evitar sobrecarga
- Balance: throughput vs overhead

### Planificador de Largo Plazo (Long-Term Scheduler)
- Decide QUÉ procesos entran al sistema
- Controla transición NEW → READY
- Gestiona recursos globales

### Diferencia con Corto Plazo
- Largo: CUÁLES procesos (admisión)
- Corto: CUÁNDO ejecutan (scheduling)

---

## Código y Demostración

### 1. Hilo del Planificador
**Archivo:** `kernel/src/planificacion/largo_plazo.c`

```c
void* hilo_planificador_largo_plazo(void* arg) {
    log_info(logger, "Planificador Largo Plazo iniciado");
    
    while (true) {
        // 1. Esperar que haya procesos nuevos
        sem_wait(&sem_hay_new);
        
        // 2. Verificar PAUSE (si está pausado, bloquear aquí)
        planificacion_check_pause();
        
        // 3. Esperar slot de multiprogramación
        sem_wait(&sem_mp);
        
        // 4. Sacar proceso de NEW
        pthread_mutex_lock(&mutex_cola_new);
        t_pcb* pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mutex_cola_new);
        
        // 5. Solicitar creación en Memoria
        if (solicitar_creacion_proceso_memoria(pcb)) {
            // 6. Mover a READY
            transicion_new_a_ready(pcb);
        } else {
            // Error: devolver slot
            sem_post(&sem_mp);
            pcb_destruir(pcb);
        }
    }
}
```

### 2. Semáforos de control
```c
// Inicialización
sem_init(&sem_hay_new, 0, 0);  // Contador de procesos en NEW
sem_init(&sem_mp, 0, config.GRADO_MULTIPROGRAMACION);  // Slots disponibles

// Ejemplo: GM = 3
// Al inicio: sem_mp = 3 (3 slots libres)
// Admite proceso 1: sem_mp = 2
// Admite proceso 2: sem_mp = 1
// Admite proceso 3: sem_mp = 0 (bloqueado hasta que termine alguno)
```

### 3. Mecanismo PAUSE/START
```c
static pthread_mutex_t mutex_pause = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_pause = PTHREAD_COND_INITIALIZER;
static bool pausado = false;

void planificacion_pausar() {
    pthread_mutex_lock(&mutex_pause);
    pausado = true;
    log_info(logger, "Planificación PAUSADA");
    pthread_mutex_unlock(&mutex_pause);
}

void planificacion_reanudar() {
    pthread_mutex_lock(&mutex_pause);
    pausado = false;
    pthread_cond_broadcast(&cond_pause);
    log_info(logger, "Planificación REANUDADA");
    pthread_mutex_unlock(&mutex_pause);
}

void planificacion_check_pause() {
    pthread_mutex_lock(&mutex_pause);
    while (pausado) {
        pthread_cond_wait(&cond_pause, &mutex_pause);
    }
    pthread_mutex_unlock(&mutex_pause);
}
```

### 4. Comunicación con Memoria
```c
bool solicitar_creacion_proceso_memoria(t_pcb* pcb) {
    log_info(logger, "Solicitando creación PID=%d a Memoria", pcb->pid);
    
    // Construir ruta completa
    char* ruta = string_from_format("%s/%s", 
        config.PATH_PROCESOS, 
        pcb->path
    );
    
    // Enviar request
    t_paquete* paq = crear_paquete(MEMORIA_CREAR_PROCESO);
    escribir_uint32(paq, pcb->pid);
    escribir_string(paq, ruta);
    enviar_paquete(paq, socket_memoria);
    
    // Esperar respuesta
    op_code respuesta = recibir_operacion(socket_memoria);
    
    free(ruta);
    destruir_paquete(paq);
    
    if (respuesta == MEMORIA_OK) {
        log_info(logger, "Memoria confirmó creación PID=%d", pcb->pid);
        return true;
    } else {
        log_error(logger, "Memoria rechazó creación PID=%d", pcb->pid);
        return false;
    }
}
```

### 5. Transición NEW → READY
```c
void transicion_new_a_ready(t_pcb* pcb) {
    pthread_mutex_lock(&pcb->mutex);
    pcb->estado = ESTADO_READY;
    pthread_mutex_unlock(&pcb->mutex);
    
    log_info(logger, "Transición: PID=%d, NEW → READY", pcb->pid);
    
    // Encolar en READY
    pthread_mutex_lock(&mutex_cola_ready);
    queue_push(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_cola_ready);
    
    // Notificar al planificador corto plazo
    sem_post(&sem_hay_ready);
}
```

---

## Demo: Grado de Multiprogramación

### Configuración (kernel.config)
```ini
GRADO_MULTIPROGRAMACION=2
```

### Escenario: 3 procesos, GM=2
```bash
so> RUN test1.txt
[OK] Proceso 1 creado
so> RUN test2.txt
[OK] Proceso 2 creado
so> RUN test3.txt
[OK] Proceso 3 creado
so> START
```

### Logs esperados
```
[KERNEL] Proceso 1 encolado en NEW
[KERNEL] Proceso 2 encolado en NEW
[KERNEL] Proceso 3 encolado en NEW
[KERNEL] Planificación reanudada
[KERNEL] LP: Admitiendo PID=1 (slot 1/2)
[MEMORIA] Creando proceso PID=1
[KERNEL] Transición: PID=1, NEW → READY
[KERNEL] LP: Admitiendo PID=2 (slot 2/2)
[MEMORIA] Creando proceso PID=2
[KERNEL] Transición: PID=2, NEW → READY
[KERNEL] LP: Esperando slot... (PID=3 bloqueado)
...
[CPU] Proceso PID=1 finalizó
[KERNEL] Liberado slot de multiprogramación (1/2 libre)
[KERNEL] LP: Admitiendo PID=3 (slot 1/2)
```

---

## Demo: PAUSE y START

### Flujo con PAUSE
```bash
so> PAUSE
[OK] Planificación pausada
so> RUN proceso1.txt
so> RUN proceso2.txt
so> RUN proceso3.txt
so> PS
PID | Estado  | Path
1   | NEW     | proceso1.txt
2   | NEW     | proceso2.txt
3   | NEW     | proceso3.txt

so> START
[OK] Planificación iniciada
# Ahora el LP empieza a admitir procesos
```

**Utilidad:**
- Cargar batch de procesos sin ejecutar
- Debugging: analizar estado antes de iniciar
- Testing: control preciso del timing

---

## Puntos Clave a Destacar

1. **Semáforos como contador:** `sem_mp` refleja slots disponibles
2. **Condition variable:** Pausa eficiente sin busy-waiting
3. **Sincronización:** Comunicación bloqueante con Memoria
4. **Robustez:** Rollback si la Memoria falla

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos el **Planificador de Corto Plazo**
- Selección del próximo proceso a ejecutar
- Quantum y timer
- Dispatch a la CPU
