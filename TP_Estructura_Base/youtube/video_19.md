# Video 19 - Recursos: WAIT y SIGNAL

**Duración estimada:** 15-20 minutos  
**Bloque:** Sincronización y Recursos

---

## Conceptos

### Semáforos
- Contador de instancias disponibles
- WAIT: decrementar (bloquea si = 0)
- SIGNAL: incrementar (desbloquea proceso)

### Sección Crítica
- Código que accede a recurso compartido
- Solo 1 proceso a la vez

### Tracking de recursos
- PCB registra recursos adquiridos
- Necesario para liberación al finalizar

---

## Código

### Estructura de recurso
```c
typedef struct {
    char* nombre;
    iint instancias;
    int instancias_totales;
    t_queue* cola_bloqueados;
    pthread_mutex_t mutex;
} t_recurso;
```

### WAIT
```c
bool recurso_wait(t_pcb* pcb, char* nombre_recurso) {
    t_recurso* recurso = dictionary_get(recursos, nombre_recurso);
    
    pthread_mutex_lock(&recurso->mutex);
    
    if (recurso->instancias > 0) {
        // Adquisición inmediata
        recurso->instancias--;
        list_add(pcb->recursos_adquiridos, string_duplicate(nombre_recurso));
        
        pthread_mutex_unlock(&recurso->mutex);
        
        log_info(logger, "WAIT: PID=%d adquirió %s (instancias=%d)", 
            pcb->pid, nombre_recurso, recurso->instancias);
        
        return true;  // No bloquea
    } else {
        // Bloquear proceso
        queue_push(recurso->cola_bloqueados, pcb);
        pcb->estado = ESTADO_BLOCKED;
        
        pthread_mutex_unlock(&recurso->mutex);
        
        log_info(logger, "WAIT: PID=%d bloqueado esperando %s", 
            pcb->pid, nombre_recurso);
        
        return false;  // Bloqueado
    }
}
```

### SIGNAL
```c
void recurso_signal(t_pcb* pcb, char* nombre_recurso) {
    t_recurso* recurso = dictionary_get(recursos, nombre_recurso);
    
    // Remover de la lista de recursos del PCB
    list_remove_by_condition(pcb->recursos_adquiridos, 
        (void*) _es_recurso_igual, nombre_recurso);
    
    pthread_mutex_lock(&recurso->mutex);
    
    if (!queue_is_empty(recurso->cola_bloqueados)) {
        // Desbloquear un proceso
        t_pcb* desbloqueado = queue_pop(recurso->cola_bloqueados);
        list_add(desbloqueado->recursos_adquiridos, string_duplicate(nombre_recurso));
        
        desbloqueado->estado = ESTADO_READY;
        queue_push(cola_ready, desbloqueado);
        sem_post(&sem_hay_ready);
        
        log_info(logger, "SIGNAL: PID=%d desbloqueó a PID=%d en %s", 
            pcb->pid, desbloqueado->pid, nombre_recurso);
    } else {
        // Incrementar instancias
        recurso->instancias++;
        
        log_info(logger, "SIGNAL: PID=%d liberó %s (instancias=%d)", 
            pcb->pid, nombre_recurso, recurso->instancias);
    }
    
    pthread_mutex_unlock(&recurso->mutex);
}
```

### Configuración
```ini
RECURSOS=[RA,RB,RC]
INSTANCIAS_RECURSOS=[1,2,1]
```

---

## Demo: Test 5

### Proceso A
```
WAIT RA
SET AX 10
SIGNAL RA
EXIT
```

### Proceso B
```
WAIT RA
SET BX 20
SIGNAL RA
EXIT
```

### Logs
```
[KERNEL] PID=1: WAIT RA
[KERNEL] WAIT: PID=1 adquirió RA (instancias=0)
[KERNEL] PID=2: WAIT RA
[KERNEL] WAIT: PID=2 bloqueado esperando RA
[CPU] PID=1: SET AX 10
[KERNEL] PID=1: SIGNAL RA
[KERNEL] SIGNAL: PID=1 desbloqueó a PID=2 en RA
[CPU] PID=2: SET BX 20
[KERNEL] PID=2: SIGNAL RA
```

---

## Puntos Clave

1. **Mutex:** Protege el recurso mismo
2. **Cola FIFO:** Orden de desbloqueo
3. **Tracking:** PCB sabe qué recursos tiene

---

## Siguiente Video

Veremos **Detección de Deadlock con Grafo de Espera**.
