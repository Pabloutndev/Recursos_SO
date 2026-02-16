# Video 4 - El PCB: Bloque de Control de Proceso

**Duración estimada:** 10-15 minutos  
**Bloque:** Gestión de Procesos

---

## Conceptos a Explicar

### ¿Qué es un PCB?
- **Process Control Block:** estructura de datos que representa un proceso
- Contiene TODA la información que el SO necesita para gestionar el proceso
- Análogo a un "expediente" del proceso

### ¿Qué información necesita el SO?
- **Identificación:** PID (Process ID)
- **Estado:** NEW, READY, EXEC, BLOCKED, EXIT
- **Contexto CPU:** registros, PC (Program Counter)
- **Recursos:** archivos abiertos, memoria asignada
- **Planificación:** prioridad, quantum, tiempos

### Importancia del PCB
- El SO no "conoce" al proceso directamente
- Toda operación se hace sobre el PCB
- Cambio de contexto = guardar/restaurar PCB

---

## Código y Demostración

### 1. Estructura del PCB
**Archivo:** `kernel/src/pcb/pcb.h`

```c
typedef enum {
    ESTADO_NEW,
    ESTADO_READY,
    ESTADO_EXEC,
    ESTADO_BLOCKED,
    ESTADO_EXIT
} t_estado_proceso;

typedef struct {
    // Identificación
    uint32_t pid;
    char* path;  // archivo de instrucciones
    
    // Estado
    t_estado_proceso estado;
    
    // Contexto CPU
    uint32_t pc;  // Program Counter
    t_registros registros;  // AX, BX, CX, DX, EAX...
    
    // Planificación
    uint32_t quantum;
    uint32_t prioridad;
    uint64_t tiempo_llegada;
    uint64_t tiempo_uso_cpu;
    uint64_t tiempo_espera;
    
    // Recursos
    t_list* recursos_adquiridos;  // [RA, RB...]
    
    // Sincronización
    pthread_mutex_t mutex;
} t_pcb;
```

### 2. Creación de PCB
**Archivo:** `kernel/src/pcb/pcb.c`

```c
static uint32_t pid_counter = 1;

t_pcb* pcb_crear(char* path) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    
    // Asignar PID único
    pcb->pid = pid_counter++;
    pcb->path = string_duplicate(path);
    
    // Estado inicial
    pcb->estado = ESTADO_NEW;
    
    // Contexto CPU inicial
    pcb->pc = 0;
    memset(&pcb->registros, 0, sizeof(t_registros));
    
    // Planificación
    pcb->quantum = config_quantum;
    pcb->prioridad = 0;  // por defecto
    pcb->tiempo_llegada = timestamp_actual();
    pcb->tiempo_uso_cpu = 0;
    pcb->tiempo_espera = 0;
    
    // Recursos
    pcb->recursos_adquiridos = list_create();
    
    // Mutex
    pthread_mutex_init(&pcb->mutex, NULL);
    
    log_info(logger, "PCB creado: PID=%d, Path=%s", pcb->pid, pcb->path);
    return pcb;
}
```

### 3. Destrucción de PCB
```c
void pcb_destruir(t_pcb* pcb) {
    log_info(logger, "Destruyendo PCB: PID=%d", pcb->pid);
    
    free(pcb->path);
    list_destroy_and_destroy_elements(
        pcb->recursos_adquiridos, 
        free
    );
    pthread_mutex_destroy(&pcb->mutex);
    free(pcb);
}
```

---

## Estados del Proceso

### Diagrama de transiciones
```
    [NEW]
      ↓
    [READY] ←─────┐
      ↓           │
    [EXEC] ───────┤
      ↓           │
    [BLOCKED] ────┘
      ↓
    [EXIT]
```

### NEW
- Proceso recién creado
- Esperando admisión (largo plazo)
- Todavía no cargado en memoria

### READY
- Listo para ejecutar
- En cola del scheduler
- Esperando asignación de CPU

### EXEC
- Ejecutando en CPU actualmente
- Solo 1 proceso en EXEC a la vez (monocore)

### BLOCKED
- Esperando evento externo
- Típicamente: I/O, WAIT de recurso
- No puede progresar aunque haya CPU libre

### EXIT
- Proceso finalizado
- Recursos siendo liberados
- PCB será destruido pronto

---

## Transiciones de Estado

### NEW → READY
- **Disparador:** Planificador largo plazo
- **Acción:** Cargar proceso en memoria
- **Condición:** Grado de multiprogramación permite

### READY → EXEC
- **Disparador:** Planificador corto plazo
- **Acción:** Restaurar contexto CPU
- **Selección:** Según algoritmo (FIFO, RR, etc.)

### EXEC → READY
- **Disparador:** Fin de quantum (desalojo)
- **Acción:** Guardar contexto CPU, encolar

### EXEC → BLOCKED
- **Disparador:** Instrucción IO, WAIT
- **Acción:** Encolar en cola del recurso/dispositivo

### BLOCKED → READY
- **Disparador:** Evento completado (IO terminó, SIGNAL)
- **Acción:** Mover a cola READY

### EXEC → EXIT
- **Disparador:** Instrucción EXIT, error, KILL
- **Acción:** Liberar memoria, recursos

---

## Demo: Ver PCBs en acción

### Código de debugging
```c
void pcb_imprimir(t_pcb* pcb) {
    printf("╔═══════════════════════════════╗\n");
    printf("║ PID: %d\n", pcb->pid);
    printf("║ Estado: %s\n", estado_a_string(pcb->estado));
    printf("║ Path: %s\n", pcb->path);
    printf("║ PC: %d\n", pcb->pc);
    printf("║ AX=%d BX=%d CX=%d DX=%d\n", 
        pcb->registros.AX,
        pcb->registros.BX,
        pcb->registros.CX,
        pcb->registros.DX
    );
    printf("║ Quantum: %d ms\n", pcb->quantum);
    printf("║ Prioridad: %d\n", pcb->prioridad);
    printf("║ Recursos: %d\n", list_size(pcb->recursos_adquiridos));
    printf("╚═══════════════════════════════╝\n");
}
```

### Ejecutar con logs
```
[KERNEL] PCB creado: PID=1, Path=test1.txt
[KERNEL] Transición: PID=1, NEW → READY
[KERNEL] Transición: PID=1, READY → EXEC
[KERNEL] Transición: PID=1, EXEC → READY (QUANTUM)
[KERNEL] Transición: PID=1, READY → EXEC
[KERNEL] Transición: PID=1, EXEC → EXIT
[KERNEL] Destruyendo PCB: PID=1
```

---

## Puntos Clave a Destacar

1. **Centralidad:** El PCB es el corazón del SO
2. **Completitud:** Contiene TODO lo necesario para reanudar ejecución
3. **Gestión:** Crear/modificar/destruir PCBs es responsabilidad exclusiva del Kernel
4. **Sincronización:** Mutex protege accesos concurrentes al PCB

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos el **ciclo completo** de un proceso
- Cómo se crea y destruye un proceso
- Interacción Kernel ↔ Memoria
- Test de ciclo básico (NEW → EXIT)
