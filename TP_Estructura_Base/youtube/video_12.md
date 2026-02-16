# Video 12 - Registros e Instrucciones

**Duración estimada:** 10-15 minutos  
**Bloque:** La CPU

---

## Conceptos a Explicar

### Registros del procesador
- Almacenamiento más rápido de la CPU
-Variables temporales del proceso
- Diferentes tamaños: 8 bits, 32 bits

### Tipos de registros
- **Propósito general:** AX-DX (8 bits), EAX-EDX (32 bits)
- **Índices:** SI, DI (32 bits)
- **Especiales:** PC (Program Counter)

---

## Código y Demostración

### 1. Estructura de registros
**Archivo:** `cpu/src/registros/registros.h`

```c
typedef struct {
    // Registros 8 bits
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    
    // Registros 32 bits
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
    
    // Índices
    uint32_t SI;  // Source Index
    uint32_t DI;  // Destination Index
} t_registros;
```

### 2. Lectura genérica
**Archivo:** `cpu/src/registros/registros.c`

```c
uint32_t registros_leer(t_registros* regs, char* nombre) {
    if (strcmp(nombre, "AX") == 0) return regs->AX;
    if (strcmp(nombre, "BX") == 0) return regs->BX;
    if (strcmp(nombre, "CX") == 0) return regs->CX;
    if (strcmp(nombre, "DX") == 0) return regs->DX;
    if (strcmp(nombre, "EAX") == 0) return regs->EAX;
    if (strcmp(nombre, "EBX") == 0) return regs->EBX;
    if (strcmp(nombre, "ECX") == 0) return regs->ECX;
    if (strcmp(nombre, "EDX") == 0) return regs->EDX;
    if (strcmp(nombre, "SI") == 0) return regs->SI;
    if (strcmp(nombre, "DI") == 0) return regs->DI;
    
    log_error(logger, "Registro desconocido: %s", nombre);
    return 0;
}
```

### 3. Escritura genérica
```c
void registros_escribir(t_registros* regs, char* nombre, uint32_t valor) {
    if (strcmp(nombre, "AX") == 0) { regs->AX = (uint8_t)valor; return; }
    if (strcmp(nombre, "BX") == 0) { regs->BX = (uint8_t)valor; return; }
    // ... resto de registros
    
    log_error(logger, "Registro desconocido: %s", nombre);
}
```

---

## Tabla Completa de Instrucciones

### Aritméticas y Lógicas
| Instrucción | Formato | Descripción |
|-------------|---------|-------------|
| SET | `SET REG VALOR` | Asignar valor a registro |
| SUM | `SUM DEST ORIGEN` | DEST = DEST + ORIGEN |
| SUB | `SUB DEST ORIGEN` | DEST = DEST - ORIGEN |

### Control de Flujo
| Instrucción | Formato | Descripción |
|-------------|---------|-------------|
| JNZ | `JNZ REG OFFSET` | Saltar si REG ≠ 0 |

###Memoria
| Instrucción | Formato | Descripción |
|-------------|---------|-------------|
| MOV_IN | `MOV_IN REG DIR_LOG` | REG = Memoria[DIR_LOG] |
| MOV_OUT | `MOV_OUT DIR_LOG REG` | Memoria[DIR_LOG] = REG |
| RESIZE | `RESIZE TAMANIO` | Cambiar tamaño proceso |

### I/O
| Instrucción | Formato | Descripción |
|-------------|---------|-------------|
| IO_GEN_SLEEP | `IO_GEN_SLEEP INTERFAZ MS` | Dormir MS milisegundos |
| IO_STDIN_READ | `IO_STDIN_READ INTERFAZ DIR TAM` | Leer de teclado |
| IO_STDOUT_WRITE | `IO_STDOUT_WRITE INTERFAZ DIR TAM` | Escribir en pantalla |
| IO_FS_CREATE | `IO_FS_CREATE INTERFAZ FILE` | Crear archivo |
| IO_FS_DELETE | `IO_FS_DELETE INTERFAZ FILE` | Eliminar archivo |
| IO_FS_WRITE | `IO_FS_WRITE INTERFAZ FILE DIR TAM OFFSET` | Escribir en archivo |
| IO_FS_READ | `IO_FS_READ INTERFAZ FILE DIR TAM OFFSET` | Leer de archivo |

### Sincronización
| Instrucción | Formato | Descripción |
|-------------|---------|-------------|
| WAIT | `WAIT RECURSO` | Adquirir recurso (bloquea si no disponible) |
| SIGNAL | `SIGNAL RECURSO` | Liberar recurso |

### Otros
| Instrucción | Formato | Descripción |
|-------------|---------|-------------|
| EXIT | `EXIT` | Finalizar proceso |

---

## WAIT y SIGNAL: Delegación al Kernel

### Implementación en CPU
```c
t_resultado ejecutar_wait(t_contexto* ctx, t_instruccion* instr) {
    char* recurso = instr->parametros[0];
    
    log_info(logger, "WAIT solicitado para recurso: %s", recurso);
    
    // La CPU NO gestiona recursos, delega al Kernel
    t_resultado res;
    res.terminar_ciclo = true;
    res.motivo = MOTIVO_WAIT;
    res.recurso = string_duplicate(recurso);
    return res;
}

t_resultado ejecutar_signal(t_contexto* ctx, t_instruccion* instr) {
    char* recurso = instr->parametros[0];
    
    log_info(logger, "SIGNAL solicitado para recurso: %s", recurso);
    
    t_resultado res;
    res.terminar_ciclo = true;
    res.motivo = MOTIVO_SIGNAL;
    res.recurso = string_duplicate(recurso);
    return res;
}
```

### Manejo en Kernel
```c
void manejar_wait(t_pcb* pcb, char* recurso) {
    log_info(logger, "Proceso PID=%d solicita WAIT(%s)", pcb->pid, recurso);
    
    if (recurso_wait(pcb, recurso)) {
        // Recurso adquirido, volver a READY
        pcb->estado = ESTADO_READY;
        queue_push(cola_ready, pcb);
        sem_post(&sem_hay_ready);
    } else {
        // Bloqueado esperando recurso
        pcb->estado = ESTADO_BLOCKED;
        // Ya está en la cola del recurso
    }
}
```

---

## Demo: Programa completo

### Archivo: `proceso_completo.txt`
```
SET AX 10
SET BX 5
SUM AX BX
MOV_OUT 0 AX
IO_STDOUT_WRITE MONITOR 0 1
WAIT RA
SET CX 100
SIGNAL RA
EXIT
```

### Trazas de ejecución
```
[CPU] SET AX 10 → AX=10
[CPU] SET BX 5 → BX=5
[CPU] SUM AX BX → AX=15
[CPU] MOV_OUT 0 AX → Memoria[0]=15
[CPU] IO_STDOUT_WRITE → Bloquea (I/O)
[KERNEL] PID desmonoblock por I/O
[IO] Escribiendo "15" en pantalla
[KERNEL] I/O completada, PID → READY
[CPU] WAIT RA → Delega al Kernel
[KERNEL] Recurso RA adquirido
[CPU] SET CX 100 → CX=100
[CPU] SIGNAL RA → Libera recurso
[KERNEL] Recurso RA liberado
[CPU] EXIT → Finaliza
```

---

## Puntos Clave a Destacar

1. **Registros:** Variables más rápidas, pero limitadas
2. **ISA custom:** Simplificado para didáctica, pero conceptualmente completo
3. **Delegación:** CPU ejecuta, Kernel coordina
4. **Completitud:** Instrucciones cubren: cómputo, memoria, I/O, sincronización

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos **interrupciones** en detalle
- Mecanismo de desalojo
- Flag de interrupción
- Tipos de interrupciones
