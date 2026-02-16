# Video 10 - Cambio de Contexto

**Duración estimada:** 10 minutos  
**Bloque:** Algoritmos de Planificación

---

## Conceptos a Explicar

### ¿Qué es el Context Switch?
- Guardar estado completo del proceso saliente
- Restaurar estado del proceso entrante
- Overhead crítico del SO

### Estado del proceso
- **Registros CPU:** AX, BX, CX, DX, EAX, etc.
- **PC (Program Counter):** próxima instrucción
- **Estado:** READY, EXEC, BLOCKED

### Costo del Context Switch
- Tiempo: típicamente 1-10 microsegundos
- Cache flush: pérdida de localidad
- TLB invalidation

---

## Código y Demostración

### 1. Enviar contexto a CPU
**Archivo:** `kernel/src/peticiones/dispatch.c`

```c
void enviar_proceso_a_cpu(t_pcb* pcb) {
    t_paquete* paq = crear_paquete(CPU_DISPATCH);
    
    // Serializar contexto
    escribir_uint32(paq, pcb->pid);
    escribir_uint32(paq, pcb->pc);
    
    // Registros 8 bits
    escribir_uint8(paq, pcb->registros.AX);
    escribir_uint8(paq, pcb->registros.BX);
    escribir_uint8(paq, pcb->registros.CX);
    escribir_uint8(paq, pcb->registros.DX);
    
    // Registros 32 bits
    escribir_uint32(paq, pcb->registros.EAX);
    escribir_uint32(paq, pcb->registros.EBX);
    escribir_uint32(paq, pcb->registros.ECX);
    escribir_uint32(paq, pcb->registros.EDX);
    escribir_uint32(paq, pcb->registros.SI);
    escribir_uint32(paq, pcb->registros.DI);
    
    enviar_paquete(paq, socket_cpu_dispatch);
    destruir_paquete(paq);
}
```

### 2. Recibir contexto en CPU
**Archivo:** `cpu/src/adaptadores/cpu_dispatch_handler.c`

```c
t_contexto* recibir_contexto() {
    t_paquete* paq = recibir_paquete(socket_dispatch);
    
    t_contexto* ctx = malloc(sizeof(t_contexto));
    
    ctx->pid = leer_uint32(paq);
    ctx->pc = leer_uint32(paq);
    
    ctx->registros.AX = leer_uint8(paq);
    ctx->registros.BX = leer_uint8(paq);
    ctx->registros.CX = leer_uint8(paq);
    ctx->registros.DX = leer_uint8(paq);
    
    ctx->registros.EAX = leer_uint32(paq);
    ctx->registros.EBX = leer_uint32(paq);
    ctx->registros.ECX = leer_uint32(paq);
    ctx->registros.EDX = leer_uint32(paq);
    ctx->registros.SI = leer_uint32(paq);
    ctx->registros.DI = leer_uint32(paq);
    
    destruir_paquete(paq);
    
    return ctx;
}
```

### 3. Manejar interrupción
**Archivo:** `kernel/src/peticiones/interrupciones.c`

```c
void manejar_interrupcion(t_pcb* pcb, t_resultado resultado) {
    switch (resultado.motivo) {
        case MOTIVO_QUANTUM:
            // Guardar contexto actualizado
            actualizar_pcb_desde_resultado(pcb, resultado);
            
            // Cambiar estado
            pcb->estado = ESTADO_READY;
            
            // Reencolar
            queue_push(cola_ready, pcb);
            sem_post(&sem_hay_ready);
            
            log_info(logger, "Context switch: PID=%d, EXEC → READY", 
                pcb->pid);
            break;
        
        // ... otros casos
    }
}
```

**Actualizar PCB:**
```c
void actualizar_pcb_desde_resultado(t_pcb* pcb, t_resultado res) {
    pcb->pc = res.pc;
    pcb->registros = res.registros;
    
    log_trace(logger, "PCB actualizado: PID=%d, PC=%d, AX=%d", 
        pcb->pid, pcb->pc, pcb->registros.AX);
}
```

---

## Diagrama de secuencia completo

```
Kernel                CPU               Memoria
  |                    |                   |
  |-- dispatch ------->|                   |
  |   (contexto)       |                   |
  |                    |-- fetch --------->|
  |                    |<-- instrucción ---|
  |                    | ejecuta           |
  |                    |                   |
  |<-- interrupción ---|                   |
  |   (contexto)       |                   |
  |                    |                   |
  | guarda PCB         |                   |
  | selecciona nuevo   |                   |
  |-- dispatch ------->|                   |
```

---

## Demo: Logs de context switch

```
[KERNEL] Despachando PID=1: PC=0, AX=0, BX=0
[CPU] Recibido contexto PID=1
[CPU] Ejecutando instrucciones...
[KERNEL] Quantum expirado: PID=1
[CPU] Devolviendo PID=1: PC=5, AX=10, BX=20
[KERNEL] Context switch: PID=1, EXEC → READY
[KERNEL] Despachando PID=2: PC=0, AX=0, BX=0
[CPU] Recibido contexto PID=2
...
[KERNEL] Despachando PID=1: PC=5, AX=10, BX=20
[CPU] Recibido contexto PID=1 (continúa donde quedó)
```

---

## Puntos Clave a Destacar

1. **Completitud:** TODO el estado viaja por la red
2. **Atomicidad:** El cambio es instantáneo (desde el punto de vista del proceso)
3. **Overhead:** Por eso el quantum no puede ser demasiado corto
4. **Transparencia:** El proceso no "sabe" que fue suspendido

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos la **CPU en detalle**
- Ciclo Fetch-Decode-Execute
- Cómo se ejecutan las instrucciones
