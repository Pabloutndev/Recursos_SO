# Modulos del TP - Sistema Operativo

## Que es esto?

Un simulador de Sistema Operativo hecho en C. Son 5 programas separados que se comunican por sockets TCP, cada uno simula una parte del SO real.

```
Consola ──TCP──> Kernel ──TCP──> CPU
                   |               |
                   |               v
                   +───────────> Memoria
                   |
                   v
              Entrada/Salida
```

---

## Los 5 modulos

### Kernel
El cerebro. Recibe comandos del usuario (via Consola) y decide que proceso ejecutar.

**Que hace:**
- Crea procesos, les asigna un PID y los pone en cola
- Decide quien ejecuta y cuando (planificacion)
- Maneja recursos compartidos (WAIT/SIGNAL como semaforos)
- Detecta deadlocks automaticamente (Grafo de Espera + Banquero)

**Algoritmos de planificacion:** FIFO, Round Robin, Virtual Round Robin, HRRN, Prioridades

**Config:** `kernel.config`

---

### CPU
Simula el procesador. Ejecuta las instrucciones de un proceso.

**Ciclo:** Fetch (pide instruccion a Memoria) -> Decode (parsea) -> Execute (ejecuta)

**Instrucciones:** SET, SUM, SUB, JNZ, MOV_IN, MOV_OUT, RESIZE, COPY_STRING, WAIT, SIGNAL, IO_GEN_SLEEP, IO_STDIN_READ, IO_STDOUT_WRITE, IO_FS_*, EXIT

**Registros:** AX/BX/CX/DX (8 bits), EAX/EBX/ECX/EDX (32 bits), SI/DI, PC

Tiene TLB (cache de traducciones de direcciones) con algoritmos FIFO o LRU.

**Config:** `cpu.config`

---

### Memoria
Simula la RAM. Guarda instrucciones y datos de los procesos.

**Dos esquemas (configurable):**
- **Paginacion:** tabla de paginas por proceso, bitmap de frames, reemplazo Clock, swap a disco
- **Segmentacion:** segmento contiguo por proceso (base + limite), First-Fit, compactacion

**Config:** `memoria.config` (tamano RAM, tamano pagina, esquema, retardo)

---

### Entrada/Salida
Simula dispositivos de I/O. Se puede levantar varias veces, una por interfaz.

**4 tipos:**
- **GENERICA** - solo bloquea un tiempo (ej: `IO_GEN_SLEEP SLEEP 3`)
- **STDIN** - lee texto del teclado y lo escribe en memoria del proceso
- **STDOUT** - lee de memoria del proceso y lo muestra en pantalla
- **DIALFS** - filesystem basado en bloques (crear, borrar, leer, escribir archivos)

---

### Consola
Cliente interactivo para controlar el Kernel.

**Comandos:** `RUN archivo [prioridad]`, `KILL pid`, `PS`, `START`, `PAUSE`, `ALGORITMO alg`, `DESALOJAR pid`, `HELP`, `EXIT`

---

### Utils (biblioteca compartida)
No es ejecutable. Es un `.so` que usan todos los modulos: protocolo de paquetes, serializacion, conexiones TCP, op_codes.

---

## Como levantar

1. **Memoria** (primero, es servidor)
2. **CPU** (se conecta a Memoria)
3. **Kernel** (se conecta a Memoria y CPU)
4. **Entrada/Salida** (se conecta a Kernel y Memoria)
5. **Consola** (se conecta a Kernel)

O directamente: `cd tests && bash run_all.sh` (levanta todo y corre 11 tests automaticos)
