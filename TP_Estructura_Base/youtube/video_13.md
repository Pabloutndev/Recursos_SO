# Video 13 - Interrupciones y Desalojo

**Duración estimada:** 10-15 minutos  
**Bloque:** La CPU

---

## Conceptos a Explicar

### Interrupciones
- Señal que interrumpe la ejecución normal
- Permite al SO retomar control de la CPU
- **Hardware:** timer, dispositivos I/O
- **Software:** system calls, excepciones

### Mecanismo de interrupción
- Flag compartido entre hilos
- CPU chequea antes de cada fetch
- Tipos: QUANTUM, KILL, etc.

### Desalojo (Preemption)
- Sacar proceso de CPU forzosamente
- Necesario para time-sharing
- CPU devuelve contexto al Kernel

---

## Código y Demostración

### 1. Flag de interrupción
**Archivo:** `cpu/src/interrupciones/interrupciones.h`

```c
typedef enum {
    INTERRUPCION_QUANTUM,
    INTERRUPCION_KILL,
    INTERRUPCION_USUARIO
} tipo_interrupcion;

// Flag compartido (volatile para evitar optimizaciones)
volatile bool hay_interrupcion;
tipo_interrupcion interrupcion_tipo;
pthread_mutex_t mutex_interrupcion;
```

### 2. Setear interrupción
**Archivo:** `cpu/src/interrupciones/interrupciones.c`

```c
void interrupcion_set(tipo_interrupcion tipo) {
    pthread_mutex_lock(&mutex_interrupcion);
    
    hay_interrupcion = true;
    interrupcion_tipo = tipo;
    
    log_info(logger, "Interrupción seteada: %s", 
        tipo_interrupcion_str(tipo));
    
    pthread_mutex_unlock(&mutex_interrupcion);
}

bool interrupcion_pendiente() {
    pthread_mutex_lock(&mutex_interrupcion);
    bool pendiente = hay_interrupcion;
    pthread_mutex_unlock(&mutex_interrupcion);
    
    return pendiente;
}

tipo_interrupcion interrupcion_get_y_limpiar() {
    pthread_mutex_lock(&mutex_interrupcion);
    
    tipo_interrupcion tipo = interrupcion_tipo;
    hay_interrupcion = false;
    
    pthread_mutex_unlock(&mutex_interrupcion);
    
    return tipo;
}
```

### 3. Hilo de interrupciones en CPU
**Archivo:** `cpu/src/server/cpu_server.c`

```c
void* hilo_interrupt(void* arg) {
    log_info(logger, "Hilo de interrupciones iniciado");
    
    while (true) {
        // Esperar interrupción del Kernel
        op_code op = recibir_operacion(socket_interrupt);
        
        if (op == CPU_INTERRUPCION_QUANTUM) {
            log_info(logger, "Recibida interrupción QUANTUM");
            interrupcion_set(INTERRUPCION_QUANTUM);
        }
        else if (op == CPU_INTERRUPCION_KILL) {
            log_info(logger, "Recibida interrupción KILL");
            interrupcion_set(INTERRUPCION_KILL);
        }
        else if (op == CPU_INTERRUPCION_USUARIO) {
            log_info(logger, "Recibida interrupción USUARIO");
            interrupcion_set(INTERRUPCION_USUARIO);
        }
    }
    
    return NULL;
}
```

### 4. Chequeo en el ciclo
**Archivo:** `cpu/src/ciclo_instruccion/ciclo.c`

```c
t_resultado ciclo_instruccion_ejecutar(t_contexto* ctx) {
    while (true) {
        // CRÍTICO: chequear ANTES de fetch
        if (interrupcion_pendiente()) {
            tipo_interrupcion tipo = interrupcion_get_y_limpiar();
            
            log_info(logger, "Procesando interrupción: %s", 
                tipo_interrupcion_str(tipo));
            
            t_resultado res;
            res.terminar_ciclo = true;
            
            if (tipo == INTERRUPCION_QUANTUM) {
                res.motivo = MOTIVO_QUANTUM;
            } else if (tipo == INTERRUPCION_KILL) {
                res.motivo = MOTIVO_EXIT;  // Finalizar forzado
            } else {
                res.motivo = MOTIVO_USUARIO;
            }
            
            return res;
        }
        
        // Fetch, decode, execute...
    }
}
```

### 5. Envío de interrupción desde Kernel
**Archivo:** `kernel/src/peticiones/interrupciones.c`

```c
void enviar_interrupcion_cpu(tipo_interrupcion tipo) {
    log_info(logger, "Enviando interrupción a CPU: %s", 
        tipo_interrupcion_str(tipo));
    
    op_code op;
    
    if (tipo == INTERRUPCION_QUANTUM) {
        op = CPU_INTERRUPCION_QUANTUM;
    } else if (tipo == INTERRUPCION_KILL) {
        op = CPU_INTERRUPCION_KILL;
    } else {
        op = CPU_INTERRUPCION_USUARIO;
    }
    
    t_paquete* paq = crear_paquete(op);
    enviar_paquete(paq, socket_cpu_interrupt);
    destruir_paquete(paq);
}
```

---

## Motivos de desalojo

### 1. EXIT (normal)
```c
// Proceso ejecutó EXIT
res.motivo = MOTIVO_EXIT;
```

### 2. QUANTUM (timer)
```c
// El timer expiró
enviar_interrupcion_cpu(INTERRUPCION_QUANTUM);
res.motivo = MOTIVO_QUANTUM;
```

### 3. I/O (voluntary)
```c
// Proceso pidió I/O
res.motivo = MOTIVO_IO;
res.dispositivo = "TECLADO";
res.tiempo = 1000;
```

### 4. SEGFAULT (error)
```c
// Acceso inválido a memoria
res.motivo = MOTIVO_SEGFAULT;
res.direccion = 0xFFFFFFFF;
```

---

## Demo: Desalojo por quantum

### Logs esperados
```
[KERNEL] Despachando PID=1, quantum=50ms
[KERNEL] Timer iniciado
[CPU] Recibido contexto PID=1
[CPU] Fetch: PC=0
[CPU] Execute: SET AX 1
[CPU] Chequeo interrupción: no
[CPU] Fetch: PC=1
[CPU] Execute: SUM AX AX
... (50ms pasan)
[KERNEL] Timer: quantum expirado
[KERNEL] Enviando interrupción QUANTUM a CPU
[CPU] Recibida interrupción QUANTUM
[CPU] Chequeo interrupción: SÍ (QUANTUM)
[CPU] Devolviendo PID=1, motivo=QUANTUM, PC=10
[KERNEL] Proceso desalojado, EXEC → READY
```

---

## Decisión del opcode de respuesta

**Archivo:** `cpu/src/adaptadores/cpu_dispatch_handler.c`

```c
void devolver_resultado_a_kernel(t_contexto* ctx, t_resultado res) {
    op_code op;
    
    switch (res.motivo) {
        case MOTIVO_EXIT:
            op = CPU_EXIT;
            break;
        
        case MOTIVO_QUANTUM:
            op = CPU_QUANTUM;
            break;
        
        case MOTIVO_IO:
            op = CPU_IO;
            break;
        
        case MOTIVO_WAIT:
            op = CPU_WAIT;
            break;
        
        case MOTIVO_SIGNAL:
            op = CPU_SIGNAL;
            break;
        
        case MOTIVO_SEGFAULT:
            op = CPU_SEGFAULT;
            break;
    }
    
    // Enviar opcode
    t_paquete* paq = crear_paquete(op);
    
    // Serializar contexto actualizado
    serializar_contexto(paq, ctx);
    
    // Datos adicionales según motivo
    if (res.motivo == MOTIVO_IO) {
        escribir_string(paq, res.dispositivo);
        escribir_uint32(paq, res.tiempo);
    }
    
    enviar_paquete(paq, socket_dispatch);
    destruir_paquete(paq);
}
```

---

## Diagrama de secuencia: Interrupción

```
Timer (Kernel)     CPU Interrupt     CPU Dispatch
     |                  |                 |
     | (quantum expire) |                 |
     |―――――――→ setea flag|                 |
     |                  |                 |
     |                  | chequea flag    |
     |                  |←―――――――――――――――――|
     |                  | devuelve resultado
     |                  |―――――――――――――――――→|
     |                  |                 |
```

---

## Puntos Clave a Destacar

1. **Asincronía:** El timer y el dispatch son hilos separados
2. **Volatile:** Necesario para compartir flag entre hilos
3. **Timing:** Se chequea ANTES de fetch (no durante execute)
4. **Tipos:** Diferentes motivos de interrupción tienen diferente manejo

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Entramos en **Gestión de Memoria**
- Conceptos de memoria virtual
- Arquitectura del módulo Memoria
- Espacio de direcciones
