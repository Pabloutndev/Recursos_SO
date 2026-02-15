# TEORIA DE SISTEMAS OPERATIVOS - Guia de Estudio

> Basado en Stallings ("Operating Systems: Internals and Design Principles") y Silberschatz/Galvin/Gagne ("Operating System Concepts"). Relacionado con la implementacion del TP.

---

## 1. INTRODUCCION A LOS SISTEMAS OPERATIVOS

### Que es un SO

Un Sistema Operativo es el software que actua como intermediario entre el usuario y el hardware. Sus funciones principales son:

- **Gestion de recursos**: CPU, memoria, dispositivos I/O, archivos
- **Abstraccion del hardware**: ofrece una interfaz uniforme sobre hardware diverso
- **Aislamiento y proteccion**: cada proceso cree tener la maquina para si mismo
- **Multiprogramacion**: permite ejecutar multiples procesos concurrentemente

### Componentes principales

| Componente | Funcion |
|------------|---------|
| **Kernel** | Nucleo del SO, gestiona procesos, memoria, I/O |
| **Shell** | Interfaz con el usuario (CLI o GUI) |
| **System Calls** | API para que programas soliciten servicios al kernel |
| **Drivers** | Controladores especificos para cada dispositivo |

### Estructuras de SO

- **Monolitico**: Todo el kernel en un solo bloque (Linux). Rapido pero dificil de mantener.
- **Microkernel**: Kernel minimo, servicios en espacio de usuario (Minix). Modular pero mas lento por IPC.
- **Modular**: Kernel con modulos cargables dinamicamente. Combina ventajas.
- **Hibrido**: Combina monolitico con microkernel (Windows, macOS).

### En el TP

El proyecto usa una **arquitectura modular** con 5 procesos independientes (Kernel, CPU, Memoria, Entrada/Salida, Consola) comunicados por sockets TCP. Esto simula la separacion de responsabilidades de un SO real, donde cada modulo tiene su propia memoria y se comunica via IPC.

---

## 2. PROCESOS

### Proceso vs Programa

- **Programa**: codigo ejecutable almacenado en disco (estatico)
- **Proceso**: instancia de un programa en ejecucion (dinamico). Incluye codigo, datos, stack, heap, registros, PC

### Estados de un proceso

```
        admit          dispatch
NEW ---------> READY ----------> RUNNING
                 ^                  |
                 |   interrupt/     |
                 |   quantum        |
                 +------------------+
                 |                  |
                 |    I/O done      | I/O request
                 +-- BLOCKED <------+
                                    |
                                    | exit
                                    v
                                  EXIT
```

| Transicion | Causa |
|------------|-------|
| NEW -> READY | El SO admite el proceso (grado de multiprogramacion lo permite) |
| READY -> RUNNING | El planificador de corto plazo lo selecciona |
| RUNNING -> READY | Interrupcion, fin de quantum (preemption) |
| RUNNING -> BLOCKED | Solicita I/O, WAIT sobre recurso |
| BLOCKED -> READY | I/O completada, SIGNAL libera recurso |
| RUNNING -> EXIT | Instruccion EXIT, error fatal, KILL |

### PCB (Process Control Block)

Estructura que contiene toda la informacion de un proceso:

| Campo | Descripcion |
|-------|-------------|
| PID | Identificador unico del proceso |
| Estado | NEW, READY, RUNNING, BLOCKED, EXIT |
| Program Counter (PC) | Proxima instruccion a ejecutar |
| Registros | Valores de todos los registros de CPU |
| Info de planificacion | Prioridad, quantum, tiempo en ready |
| Info de memoria | Tabla de paginas, segmentos, limites |
| Info de I/O | Dispositivos asignados, archivos abiertos |

### Context Switch

Cuando el SO cambia de un proceso a otro:

1. Guardar el estado del proceso actual (registros, PC) en su PCB
2. Cargar el estado del nuevo proceso desde su PCB
3. Actualizar estructuras del SO (colas, tablas)

**Costo**: El context switch es overhead puro. No se hace trabajo util durante el cambio. Incluye flush de TLB/cache.

### En el TP

- **PCB**: Definido en `kernel/src/pcb/pcb.h` con pid, pc, registros (AX, BX, CX, DX, EAX, EBX, ECX, EDX, SI, DI), estado, quantum, quantum_restante, tiempo_ready, tiempo_inicio_exec
- **Context switch**: Se serializa el contexto (registros + PC) y se envia por socket dispatch a CPU. Al terminar, CPU devuelve el contexto actualizado al Kernel.
- **Estados**: Colas separadas (cola_new, cola_ready, cola_exec, cola_blocked, cola_exit) con mutex individual

---

## 3. HILOS (THREADS)

### Concepto

Un hilo es la unidad minima de ejecucion dentro de un proceso. Multiples hilos comparten:
- Codigo, datos, archivos abiertos, heap

Cada hilo tiene su propio:
- PC, registros, stack

### Ventajas

- **Responsiveness**: Un hilo bloqueado no bloquea a los demas
- **Economia**: Crear un hilo es mas barato que crear un proceso
- **Compartir recursos**: Los hilos del mismo proceso comparten memoria naturalmente
- **Paralelismo**: En multiprocesadores, hilos corren en paralelo real

### Modelos de mapeo

| Modelo | Descripcion | Ejemplo |
|--------|-------------|---------|
| Many-to-One | N hilos usuario -> 1 hilo kernel | Green threads |
| One-to-One | 1 hilo usuario -> 1 hilo kernel | pthreads (Linux) |
| Many-to-Many | N hilos usuario -> M hilos kernel | Solaris |

### En el TP

El Kernel usa **pthreads** (one-to-one) para:
- `hilo_largo`: planificador de largo plazo (NEW -> READY)
- `hilo_corto`: planificador de corto plazo (READY -> EXEC)
- Hilo de consola: recibe comandos remotos
- Hilos de quantum: uno por proceso en EXEC, cuenta el tiempo y envia interrupcion
- Hilos de I/O: manejan conexiones con interfaces

---

## 4. PLANIFICACION DE CPU

### 4.1 Conceptos basicos

Los procesos alternan entre **CPU bursts** (ejecucion en CPU) e **I/O bursts** (espera por I/O).

**Tipos de planificador:**

| Planificador | Frecuencia | Funcion |
|-------------|------------|---------|
| Largo plazo | Baja | Controla grado de multiprogramacion (cuantos procesos admitir) |
| Corto plazo | Alta | Decide cual proceso en READY ejecutar a continuacion |
| Mediano plazo | Media | Swap in/out de procesos para controlar memoria |

**Criterios de evaluacion:**

| Criterio | Definicion | Objetivo |
|----------|-----------|----------|
| CPU Utilization | % de tiempo que CPU esta ocupada | Maximizar |
| Throughput | Procesos completados por unidad de tiempo | Maximizar |
| Turnaround Time | Tiempo total desde submission hasta completion | Minimizar |
| Waiting Time | Tiempo total en cola READY | Minimizar |
| Response Time | Tiempo desde submission hasta primera respuesta | Minimizar |

**Formulas:**
- `Turnaround = Completion - Arrival`
- `Waiting = Turnaround - Burst Time`
- `Response Ratio (HRRN) = (Waiting + Service) / Service`

**Preemptive vs Non-preemptive:**
- **Non-preemptive**: El proceso mantiene CPU hasta que termina o se bloquea voluntariamente (FIFO, SJF, HRRN)
- **Preemptive**: El SO puede quitarle la CPU al proceso (RR, SRTF, Prioridades preemptivas)

### 4.2 Algoritmos de planificacion

#### FIFO / FCFS (First Come First Served)

- **Tipo**: Non-preemptive
- **Mecanismo**: Cola FIFO simple. El primero que llega, primero se ejecuta.
- **Ventaja**: Simple de implementar
- **Desventaja**: Convoy effect - un proceso largo bloquea a todos los cortos

```
Proceso | Llegada | Burst
P1      | 0       | 10
P2      | 1       | 3
P3      | 2       | 5

Gantt:  |---P1---|--P2--|---P3---|
        0       10     13       18

Waiting:  P1=0, P2=9, P3=11  ->  Promedio = 6.67
```

#### SJF (Shortest Job First)

- **Tipo**: Non-preemptive
- **Mecanismo**: Ejecuta el proceso con menor burst time estimado
- **Ventaja**: Optimo en waiting time promedio
- **Desventaja**: Starvation de procesos largos. Imposible conocer el burst futuro (se estima)
- **Variante preemptive**: SRTF (Shortest Remaining Time First)

```
Proceso | Llegada | Burst
P1      | 0       | 10
P2      | 1       | 3
P3      | 2       | 5

Gantt:  |---P1---|P2|--P3--|  (non-preemptive, P1 ya estaba ejecutando)
        0       10  13    18

Si todos llegan a t=0:
Gantt:  |P2|--P3--|---P1---|
        0   3     8       18
Waiting:  P1=8, P2=0, P3=3  ->  Promedio = 3.67
```

#### Round Robin (RR)

- **Tipo**: Preemptive
- **Mecanismo**: FCFS con quantum. Cada proceso ejecuta hasta quantum ms, luego va al final de READY.
- **Ventaja**: Justo, buena response time, sin starvation
- **Desventaja**: Overhead por context switch. Performance depende del quantum.
  - Quantum muy grande -> Se comporta como FCFS
  - Quantum muy chico -> Demasiado overhead por context switch
  - Regla practica: quantum debe ser mayor que el 80% de los CPU bursts

```
Proceso | Llegada | Burst     Quantum = 4
P1      | 0       | 10
P2      | 0       | 3
P3      | 0       | 5

Gantt:  |P1|P2|P3|P1|P3|P1|
        0  4  7 11 15 16 18

Waiting: P1=0+3+4=7, P2=4, P3=7+4=11 -> Promedio=7.33
```

#### Virtual Round Robin (VRR)

- **Tipo**: Preemptive
- **Mecanismo**: Como RR, pero cuando un proceso vuelve de I/O, usa su **quantum restante** en vez del quantum completo. Mas justo con procesos I/O-bound.
- **Ventaja**: Procesos I/O-bound no son penalizados por haber usado poco CPU
- **Desventaja**: Mas complejo de implementar

```
P1 tiene quantum=10, ejecuta 3ms, luego I/O.
En RR: al volver, recibe quantum completo (10ms) - injusto, ya uso 3ms
En VRR: al volver, recibe quantum_restante (7ms) - justo
```

#### HRRN (Highest Response Ratio Next)

- **Tipo**: Non-preemptive
- **Mecanismo**: Calcula Response Ratio = (W + S) / S para cada proceso en READY. Ejecuta el de mayor ratio.
- **W** = tiempo esperando en READY
- **S** = burst time estimado
- **Ventaja**: Previene starvation (a mas espera, mayor ratio). Balancea procesos cortos y largos.
- **Desventaja**: Requiere estimar burst time

```
Proceso | Llegada | Burst   En t=10 (P1 termino):
P1      | 0       | 10      P2: RR = (9+3)/3 = 4.0
P2      | 1       | 3       P3: RR = (8+5)/5 = 2.6
P3      | 2       | 5       -> Ejecuta P2 (mayor ratio)
```

#### Prioridades

- Cada proceso tiene una prioridad (numero menor = mayor prioridad, o viceversa)
- **Preemptive**: Si llega un proceso de mayor prioridad, desaloja al actual
- **Non-preemptive**: Espera a que termine el actual
- **Problema**: Starvation de procesos de baja prioridad
- **Solucion**: Aging - incrementar prioridad de procesos que esperan mucho

#### Multilevel Queue / Multilevel Feedback Queue

- **Multilevel Queue**: Varias colas con diferentes algoritmos y prioridades (ej: cola interactiva con RR, cola batch con FCFS)
- **Multilevel Feedback Queue**: Los procesos pueden moverse entre colas. Un proceso CPU-bound baja de prioridad; uno I/O-bound sube.

### 4.3 En el TP

- **Largo plazo** (`largo_plazo.c`): Espera `sem_hay_new`, chequea `sem_mp` (grado multiprogramacion), mueve de `cola_new` a `cola_ready`
- **Corto plazo** (`corto_plazo.c`): Espera `sem_hay_ready`, llama al algoritmo seleccionado, envia proceso a CPU
- **FIFO**: `list_remove` de posicion 0 en `cola_ready`
- **RR**: Igual que FIFO + hilo timer con `usleep(quantum * 1000)` que envia interrupcion a CPU al vencer
- **VRR**: RR pero `pcb->quantum_restante` se calcula como `quantum - tiempo_ejecutado` cuando se bloquea por I/O. Al volver de I/O, usa ese quantum restante. Si se agota (quantum interrupt), se resetea al completo.
- **HRRN**: Recorre `cola_ready`, calcula `(W + S) / S` para cada PCB, selecciona el maximo
- **Cambio de algoritmo**: Configurable via consola con comando ALGORITMO

---

## 5. SINCRONIZACION DE PROCESOS

### 5.1 El problema de la seccion critica

Cuando multiples hilos/procesos acceden a recursos compartidos concurrentemente, pueden ocurrir **race conditions**: el resultado depende del orden de ejecucion.

**Seccion critica**: Segmento de codigo que accede a recursos compartidos. Solo un proceso/hilo debe estar en su seccion critica a la vez.

**Requisitos de solucion:**

| Requisito | Descripcion |
|-----------|-------------|
| Mutual Exclusion | Solo un proceso a la vez en la seccion critica |
| Progress | Si nadie esta en SC, la decision de quien entra no se pospone indefinidamente |
| Bounded Waiting | Hay un limite en cuantas veces otros procesos entran a SC antes que un proceso que espera |

### 5.2 Herramientas de sincronizacion

#### Mutex (Mutual Exclusion Lock)

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_lock(&mutex);    // Entrada a seccion critica (bloquea si ocupado)
// ... seccion critica ...
pthread_mutex_unlock(&mutex);  // Salida de seccion critica
```

- Solo puede ser liberado por el mismo hilo que lo adquirio
- Si el mutex esta locked, el hilo que intenta lock se bloquea

#### Semaforos

Un semaforo es un entero con dos operaciones atomicas:

- **wait(S)** (P, down): Si S > 0, decrementa S. Si S == 0, bloquea.
- **signal(S)** (V, up): Incrementa S. Si hay procesos bloqueados, despierta uno.

```c
sem_t semaforo;
sem_init(&semaforo, 0, valor_inicial);

sem_wait(&semaforo);   // P - decrementa o bloquea
// ... seccion critica o recurso ...
sem_post(&semaforo);   // V - incrementa y despierta
```

- **Semaforo binario** (valor 0 o 1): Funciona como mutex
- **Semaforo contador** (valor >= 0): Controla acceso a N instancias de un recurso

#### Condition Variables

Permiten a un hilo esperar hasta que se cumpla una condicion:

```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Hilo que espera:
pthread_mutex_lock(&mutex);
while (!condicion)
    pthread_cond_wait(&cond, &mutex);  // Libera mutex, espera, re-adquiere mutex
pthread_mutex_unlock(&mutex);

// Hilo que señaliza:
pthread_mutex_lock(&mutex);
condicion = true;
pthread_cond_signal(&cond);    // Despierta un hilo
// o pthread_cond_broadcast(&cond);  // Despierta todos
pthread_mutex_unlock(&mutex);
```

**Importante**: `pthread_cond_wait` SIEMPRE debe usarse dentro de un `while`, no un `if`, por spurious wakeups.

### 5.3 Problemas clasicos

#### Productor-Consumidor (Bounded Buffer)

- Buffer de tamanio N
- Productor: genera items, los pone en buffer (espera si lleno)
- Consumidor: saca items del buffer (espera si vacio)
- Solucion: 2 semaforos (espacios_vacios=N, items_disponibles=0) + 1 mutex

#### Lectores-Escritores

- Multiples lectores pueden leer simultaneamente
- Solo un escritor a la vez, y sin lectores
- Variantes: prioridad a lectores, prioridad a escritores, equitativo

#### Filosofos Comensales

- 5 filosofos, 5 tenedores. Cada uno necesita 2 tenedores para comer.
- Problema: deadlock si todos toman el tenedor izquierdo simultaneamente
- Soluciones: asimetria, semaforo de admision, etc.

### 5.4 Deadlock

Cuatro condiciones **necesarias** (todas deben cumplirse):

1. **Mutual exclusion**: Al menos un recurso es no compartible
2. **Hold and wait**: Un proceso tiene recursos y espera por mas
3. **No preemption**: Los recursos no se pueden quitar por fuerza
4. **Circular wait**: Cadena circular de procesos esperando recursos

**Estrategias:**

| Estrategia | Descripcion |
|-----------|-------------|
| **Prevencion** | Eliminar alguna de las 4 condiciones (ej: ordenar recursos) |
| **Evasion** | Algoritmo del banquero: verificar estado seguro antes de asignar |
| **Deteccion** | Permitir deadlock, detectarlo con grafo de recursos, recuperar |
| **Ignorar** | Algoritmo del avestruz (la mayoria de los SO comerciales) |

### 5.5 En el TP

- **Mutex por cola**: `mutex_new`, `mutex_ready`, `mutex_exec`, `mutex_blocked`, `mutex_exit`
- **Semaforos de senializacion**: `sem_hay_new` (avisa que hay procesos nuevos), `sem_hay_ready` (avisa que hay procesos listos)
- **Semaforo de multiprogramacion**: `sem_mp` inicializado en GRADO_MULTIPROGRAMACION, controla cuantos procesos pueden estar activos
- **Condition variable**: `cond_planif_resume` con `mutex_estado_planif` para implementar PAUSE/START de planificacion
- **Recursos**: WAIT/SIGNAL implementan semaforos contadores con instancias limitadas (`RECURSOS=[RA,RB,RC]`, `INSTANCIAS_RECURSOS=[1,2,1]`)
- **Socket mutex**: `mutex_socket_memoria` protege acceso concurrente al socket de memoria desde multiples hilos

---

## 6. GESTION DE MEMORIA

### 6.1 Conceptos basicos

| Concepto | Descripcion |
|----------|-------------|
| **Dir. logica** | Direccion generada por CPU (virtual). El proceso la ve. |
| **Dir. fisica** | Direccion real en RAM. El hardware la usa. |
| **MMU** | Hardware que traduce direccion logica a fisica |
| **Binding** | Asociacion entre dir. logica y fisica. Puede ser en compilacion, carga, o ejecucion. |

**Fragmentacion:**

| Tipo | Causa | Solucion |
|------|-------|----------|
| **Interna** | Espacio asignado > espacio usado (dentro de una pagina/particion) | Paginas mas chicas |
| **Externa** | Huecos libres entre procesos que no alcanzan para uno nuevo | Compactacion, paginacion |

### 6.2 Esquemas de gestion de memoria

#### Particiones contiguas

Cada proceso ocupa un bloque contiguo de memoria.
- **Fijas**: Particiones de tamanio predefinido. Simple pero desperdicio (fragmentacion interna).
- **Dinamicas**: Particiones del tamanio exacto del proceso. Sin fragmentacion interna pero con externa.

Algoritmos de asignacion para particiones dinamicas:

| Algoritmo | Estrategia | Fragmentacion |
|-----------|-----------|---------------|
| **First-Fit** | Primer hueco suficientemente grande | Rapido, buena performance |
| **Best-Fit** | Hueco mas chico que alcance | Deja huecos muy pequenios |
| **Worst-Fit** | Hueco mas grande | Deja huecos grandes pero se fragmenta |

#### Segmentacion

La memoria se divide en **segmentos** logicos (codigo, datos, stack). Cada segmento tiene:
- **Base**: direccion fisica de inicio
- **Limite**: tamanio del segmento

Traduccion: `dir_fisica = base + offset` (si `offset < limite`, sino SEGFAULT)

```
Dir. logica: (segmento, offset)

Tabla de segmentos:
| Segmento | Base   | Limite |
|----------|--------|--------|
| 0 (code) | 0x1000 | 4096   |
| 1 (data) | 0x3000 | 2048   |
| 2 (stack)| 0x5000 | 1024   |

Dir. logica (1, 500) -> Base=0x3000 + 500 = 0x31F4
Dir. logica (2, 2000) -> 2000 >= 1024 -> SEGFAULT!
```

- **Ventaja**: Refleja la estructura logica del programa, facilita proteccion y comparticion
- **Desventaja**: Fragmentacion externa (los segmentos son de tamanio variable)

#### Paginacion

La memoria se divide en bloques de tamanio fijo:
- **Paginas**: bloques del espacio logico
- **Frames** (marcos): bloques del espacio fisico
- Tamanio de pagina = tamanio de frame (tipicamente 4KB)

Traduccion: `dir_fisica = frame * tam_pagina + offset`

```
Dir. logica: | num_pagina | offset |
              bits altos    bits bajos

Ejemplo: tam_pagina = 256 bytes, dir_logica = 750

num_pagina = 750 / 256 = 2
offset     = 750 % 256 = 238

Tabla de paginas:
| Pagina | Frame | Presente | Modificado | Uso |
|--------|-------|----------|------------|-----|
| 0      | 5     | 1        | 0          | 1   |
| 1      | 8     | 1        | 1          | 1   |
| 2      | 3     | 1        | 0          | 0   |
| 3      | -     | 0        | 0          | 0   |  <- en swap

dir_fisica = 3 * 256 + 238 = 1006
```

- **Ventaja**: Sin fragmentacion externa. Cualquier frame libre sirve para cualquier pagina.
- **Desventaja**: Fragmentacion interna en la ultima pagina. Tabla de paginas consume memoria.

### 6.3 En el TP

Capa de abstraccion `esquema_memoria` (`esquema_memoria.h/c`) que permite elegir entre PAGINACION y SEGMENTACION via config:

```
# memoria.config
ESQUEMA_MEMORIA=PAGINACION   # o SEGMENTACION
```

**Paginacion** (`paginas.c`):
- Tabla de paginas por proceso en `t_dictionary`
- Frames gestionados con bitmap (`t_bitarray`) en `frames.c`
- Cada entrada: frame, presente, modificado, uso
- RAM simulada como `malloc(TAM_MEMORIA)` en `memoria_ram.c`

**Segmentacion** (`segmentacion.c`):
- Un segmento contiguo por proceso
- Free list (`t_list` de bloques libres) con first-fit
- Merge de bloques adyacentes al liberar
- Traduccion: base + offset con validacion de limite
- Resize con reubicacion si no hay espacio adyacente

---

## 7. MEMORIA VIRTUAL

### 7.1 Demand Paging

No todas las paginas de un proceso necesitan estar en RAM. Se cargan **bajo demanda**:
1. Proceso intenta acceder a pagina con bit `presente = 0`
2. **Page fault**: CPU genera excepcion
3. SO busca la pagina en swap (disco)
4. Si hay frame libre, carga la pagina ahi
5. Si NO hay frame libre, ejecuta **algoritmo de reemplazo**
6. Actualiza tabla de paginas y reinicia la instruccion

### 7.2 Algoritmos de reemplazo de paginas

#### FIFO

- Reemplaza la pagina que llego primero a RAM
- Simple (cola FIFO de frames)
- **Anomalia de Belady**: Mas frames puede dar MAS page faults (contraintuitivo)

#### Optimo (OPT)

- Reemplaza la pagina que no sera usada por mas tiempo en el futuro
- Imposible de implementar (requiere conocer el futuro)
- Sirve como benchmark para comparar otros algoritmos

#### LRU (Least Recently Used)

- Reemplaza la pagina menos recientemente usada
- Buena aproximacion de OPT (pasado reciente predice futuro cercano)
- Costoso de implementar exactamente (requiere timestamp o stack por cada acceso)

#### Clock (Segunda Oportunidad)

Aproximacion eficiente de LRU:

```
Puntero circular ("manecilla") recorre los frames:

Frame: [0] [1] [2] [3] [4] [5]
Uso:    1   0   1   1   0   0
        ^--- manecilla

Algoritmo al buscar victima:
1. Si uso == 1: poner uso = 0, avanzar
2. Si uso == 0: VICTIMA! Reemplazar esta pagina
3. Repetir circularmente

Paso 1: Frame 0, uso=1 -> uso=0, avanzar
Paso 2: Frame 1, uso=0 -> VICTIMA! Reemplazar frame 1
```

- La manecilla **persiste** entre page faults (no reinicia)
- Es O(n) en peor caso pero O(1) amortizado

#### Enhanced Clock (NRU - Not Recently Used)

Usa 2 bits: (referencia, modificado). Prioridad de reemplazo:
1. (0,0) - ni usado ni modificado (mejor victima)
2. (0,1) - no usado pero modificado
3. (1,0) - usado pero no modificado
4. (1,1) - usado y modificado (peor victima)

### 7.3 Thrashing

Cuando un proceso no tiene suficientes frames, genera page faults constantemente. El sistema pasa mas tiempo haciendo swap que ejecutando.

**Deteccion**: Alta tasa de page faults + CPU utilization baja
**Solucion**: Reducir grado de multiprogramacion, dar mas frames al proceso, modelo de working set

### 7.4 TLB (Translation Lookaside Buffer)

Cache de hardware para traducciones recientes (pagina -> frame). Evita consultar la tabla de paginas en cada acceso.

```
Acceso a memoria con TLB:

1. CPU genera dir. logica (pagina, offset)
2. Buscar pagina en TLB:
   - TLB HIT: Obtener frame directamente (rapido, ~1ns)
   - TLB MISS: Buscar en tabla de paginas en RAM (~100ns), cargar en TLB
3. dir_fisica = frame * tam_pagina + offset
4. Acceder a RAM con dir_fisica
```

**Effective Access Time (EAT):**

```
EAT = hit_ratio * (tlb_time + mem_time) + (1 - hit_ratio) * (tlb_time + 2 * mem_time)

Ejemplo: hit_ratio=0.98, tlb_time=1ns, mem_time=100ns
EAT = 0.98 * (1+100) + 0.02 * (1+200) = 98.98 + 4.02 = 103ns
(vs 200ns sin TLB)
```

**Politicas de reemplazo de TLB:**
- FIFO: reemplaza la entrada mas antigua
- LRU: reemplaza la menos recientemente usada

### 7.5 Swap

Cuando no hay frames libres:
1. Seleccionar victima (algoritmo de reemplazo)
2. Si pagina fue **modificada** (dirty bit = 1), escribirla a swap (disco)
3. Si no fue modificada, solo marcarla como no presente (ya esta en swap o nunca cambio)
4. Liberar el frame
5. Cargar la pagina nueva en ese frame

### 7.6 En el TP

- **Page faults**: `paginacion_obtener_entrada` trae paginas desde swap si `presente == 0`
- **Reemplazo Clock**: Implementado en `reemplazo.c` con `clock_hand` estatico (puntero circular persistente)
- **Swap**: Archivo en disco por proceso en `swap.c`. Guarda paginas desalojadas.
- **TLB** en CPU: Cache de `(pid, pagina) -> frame` en `tlb.c`. Configurable con FIFO o LRU via config `ALGORITMO_TLB`
- **Flush de TLB**: `tlb_clear_pid(pid)` limpia solo las entradas del PID viejo en context switch

---

## 8. SISTEMA DE ARCHIVOS

### 8.1 Conceptos

- **Archivo**: Unidad logica de almacenamiento con nombre. Secuencia de bytes o registros.
- **Directorio**: Estructura que organiza archivos. Puede ser arbol, grafo.
- **Metadatos**: Nombre, tamanio, permisos, fechas, ubicacion en disco

### 8.2 Metodos de asignacion de espacio en disco

| Metodo | Descripcion | Ventaja | Desventaja |
|--------|------------|---------|------------|
| **Contigua** | Bloques consecutivos | Acceso secuencial y directo rapido | Fragmentacion externa |
| **Enlazada** | Cada bloque apunta al siguiente | Sin fragmentacion | Solo acceso secuencial, punteros ocupan espacio |
| **FAT** | Tabla separada con enlaces | Acceso directo posible via tabla | Tabla puede ser grande |
| **Indexada** | Bloque indice con punteros a todos los bloques | Acceso directo eficiente | Overhead del bloque indice |

### 8.3 Gestion de espacio libre

- **Bitmap**: Un bit por bloque (0=libre, 1=ocupado). Eficiente en espacio y busqueda.
- **Lista enlazada**: Lista de bloques libres. Simple pero lento para buscar.
- **Agrupacion**: Primer bloque libre contiene direcciones de N bloques libres.

### 8.4 En el TP

**DIALFS** (`dialfs.c`):
- Filesystem simple basado en bloques contiguos
- Bitmap de bloques libres para rastrear espacio disponible
- Metadata por archivo: nombre, bloque_inicio, tamanio_en_bloques
- Operaciones: create (asignar bloques), delete (liberar bloques), truncate (redimensionar), read/write (transferir datos entre archivo y memoria del proceso)
- Configuracion: `BLOCK_SIZE`, `BLOCK_COUNT`, `PATH_BASE_DIALFS`

---

## 9. ENTRADA/SALIDA (I/O)

### 9.1 Fundamentos teoricos

La Entrada/Salida es uno de los pilares de un SO. Silberschatz (Cap. 13) y Stallings (Cap. 11) la definen como todo mecanismo por el cual un proceso intercambia datos con el mundo exterior: dispositivos fisicos, archivos, red, usuario.

**Problema central**: Los dispositivos de I/O son **ordenes de magnitud mas lentos** que la CPU. Un acceso a disco toma ~10ms, mientras que la CPU ejecuta instrucciones en ~1ns. Si la CPU esperara cada I/O, estaria ociosa el 99.99% del tiempo.

**Solucion del SO**: Cuando un proceso solicita I/O, el SO lo **bloquea** (RUNNING -> BLOCKED) y pone otro proceso a ejecutar. Cuando el dispositivo completa la operacion, el SO **desbloquea** al proceso (BLOCKED -> READY). Esto maximiza la utilizacion de CPU.

```
Sin multiprogramacion:          Con multiprogramacion:

CPU: [P1 run][  wait  ][P1 run] CPU: [P1 run][P2 run][P1 run][P2 run]
I/O:        [P1  I/O  ]        I/O:         [P1 I/O]       [P2 I/O]
             ^CPU ociosa^                    ^CPU siempre ocupada^
```

### 9.2 Clasificacion de dispositivos

**Por tipo de acceso:**

| Tipo | Acceso | Unidad | Ejemplos |
|------|--------|--------|----------|
| **Por bloques** | Aleatorio (seek + read) | Bloque de N bytes | Disco, SSD, USB, CD |
| **Por caracteres** | Secuencial (stream) | Byte a byte | Teclado, mouse, serial, impresora |
| **Red** | Ambos (sockets) | Paquetes/bytes | NIC (placa de red) |

**Por direccion de datos:**

| Direccion | Operacion | Ejemplo |
|-----------|-----------|---------|
| **Entrada (Input)** | Dispositivo -> Memoria | Teclado, microfono, sensor, disco (lectura) |
| **Salida (Output)** | Memoria -> Dispositivo | Pantalla, parlante, impresora, disco (escritura) |
| **Bidireccional** | Ambas | Disco, NIC, terminal |

### 9.3 Capas del subsistema de I/O

Stallings describe la arquitectura de I/O en capas (de arriba a abajo):

```
+------------------------------------------+
|  Proceso de usuario                      |  <- IO_GEN_SLEEP, IO_STDIN_READ, etc.
+------------------------------------------+
|  Kernel: Subsistema de I/O               |  <- Scheduling, buffering, caching
+------------------------------------------+
|  Driver del dispositivo                  |  <- Traduce operaciones genericas a especificas
+------------------------------------------+
|  Controlador del dispositivo (hardware)  |  <- Registros de control, DMA
+------------------------------------------+
|  Dispositivo fisico                      |  <- Mecanismo real (motor disco, LED pantalla)
+------------------------------------------+
```

**El TP simula las capas superiores**: el proceso ejecuta instrucciones de I/O, el Kernel actua como subsistema de I/O (scheduling, bloqueo), y el modulo Entrada/Salida simula el driver + dispositivo.

### 9.4 Tecnicas de I/O

| Tecnica | Mecanismo | CPU durante I/O | Uso |
|---------|-----------|-----------------|-----|
| **I/O Programada** | CPU hace polling (busy waiting), verifica estado del dispositivo en loop | Ocupada 100% | Microcontroladores simples |
| **I/O por interrupciones** | CPU inicia operacion y sigue trabajando. Dispositivo interrumpe al completar | Libre | Dispositivos lentos (teclado) |
| **DMA (Direct Memory Access)** | Controlador DMA transfiere bloque completo a RAM sin CPU. Interrumpe al final | Libre | Dispositivos rapidos (disco, red) |

```
I/O Programada:
CPU: [iniciar][poll][poll][poll][poll][leer dato][continuar]
                ^--- CPU desperdiciada ---^

I/O por Interrupciones:
CPU: [iniciar][hacer otra cosa...][INT!][leer dato][continuar]
                                   ^--- interrupcion del dispositivo

DMA:
CPU: [iniciar DMA][hacer otra cosa...][INT!][continuar]
DMA:              [transferir datos a RAM]  ^--- todo ya esta en RAM
```

### 9.5 Conceptos clave del subsistema de I/O

**Buffering (Almacenamiento intermedio)**:
- Zona de memoria temporal entre productor y consumidor de datos
- **Single buffer**: SO mantiene un buffer. Mientras se llena el siguiente, se procesa el actual.
- **Double buffer**: Dos buffers alternados. Mientras uno se llena, el otro se procesa. Mas eficiente.
- **Circular buffer**: N buffers en anillo. Maximiza concurrencia.
- Ejemplo: sin buffer, cada byte del teclado causaria una interrupcion. Con buffer, se acumulan bytes y se procesan juntos.

**Caching**:
- Copia de datos frecuentes en memoria rapida
- Diferencia con buffer: el buffer tiene la **unica** copia; el cache tiene una copia **adicional**
- Ejemplo: TLB es un cache de traducciones, disk cache acelera lecturas repetidas

**Spooling**:
- Cola de trabajos para dispositivos que no permiten acceso concurrente
- Ejemplo: impresora. Multiples procesos envian trabajos al spool, se imprimen secuencialmente.
- El spool desacopla la velocidad del proceso de la velocidad del dispositivo

### 9.6 El bloqueo por I/O y su relacion con la planificacion

Este es el concepto clave que une I/O con planificacion (Silberschatz Cap. 5-6):

**Tipos de procesos segun su comportamiento:**

| Tipo | Comportamiento | CPU burst | I/O frequency |
|------|---------------|-----------|---------------|
| **CPU-bound** | Calcula mucho, poca I/O | Largos | Baja |
| **I/O-bound** | Mucha I/O, poco calculo | Cortos | Alta |

**Flujo completo de una operacion de I/O:**

```
1. Proceso en RUNNING ejecuta instruccion de I/O
   (ej: IO_STDIN_READ TECLADO 0x100 64)

2. CPU detecta que es instruccion de I/O
   -> Devuelve contexto al Kernel con motivo "IO"
   -> Incluye: nombre interfaz, parametros de la operacion

3. Kernel recibe el contexto
   -> Cambia estado del proceso: RUNNING -> BLOCKED
   -> Agrega proceso a cola_blocked
   -> Busca la interfaz por nombre
   -> Envia el pedido de I/O a la interfaz via socket

4. Planificador de corto plazo toma otro proceso de READY
   -> Nuevo proceso pasa a RUNNING
   -> La CPU no se queda ociosa

5. Interfaz de I/O ejecuta la operacion
   -> GENERICA: usleep(tiempo)
   -> STDIN: lee del teclado, escribe en memoria del proceso
   -> STDOUT: lee de memoria del proceso, muestra en pantalla
   -> DIALFS: opera sobre el filesystem

6. Interfaz termina y notifica al Kernel (FIN_IO)

7. Kernel recibe FIN_IO
   -> Busca proceso en cola_blocked
   -> Cambia estado: BLOCKED -> READY
   -> Agrega a cola_ready
   -> Senializa sem_hay_ready

8. Planificador eventualmente selecciona el proceso de READY
   -> Proceso vuelve a RUNNING, continua desde donde quedo
```

**Impacto en VRR**: Cuando un proceso se bloquea por I/O con VRR, se calcula cuanto quantum consumio (`quantum_restante = quantum - tiempo_ejecutado`). Al volver de I/O, usa ese quantum restante en vez del completo. Esto es mas justo con procesos I/O-bound que frecuentemente ceden la CPU voluntariamente.

### 9.7 Tipos de interfaz en detalle

#### GENERICA - Dispositivo con latencia

**Teoria**: Representa cualquier dispositivo que simplemente introduce un retardo. En un SO real, esto modela dispositivos como un timer, un sleep del proceso, o cualquier dispositivo cuya unica caracteristica relevante es el tiempo que tarda en responder.

**Concepto**: El proceso solicita "dormir" N unidades de tiempo. Durante ese tiempo, el proceso esta BLOCKED y la CPU ejecuta otros procesos. Es la forma mas simple de I/O: no transfiere datos, solo introduce latencia.

**Mecanismo**:
```
Instruccion: IO_GEN_SLEEP <nombre_interfaz> <unidades_de_trabajo>

1. CPU ejecuta IO_GEN_SLEEP SLEEP 5
2. Kernel bloquea el proceso, envia pedido a interfaz "SLEEP"
3. Interfaz calcula: tiempo = unidades * TIEMPO_UNIDAD_TRABAJO (del config)
   -> 5 * 500ms = 2500ms
4. Interfaz ejecuta usleep(2500000)  (2.5 segundos)
5. Al despertar, interfaz envia FIN_IO al Kernel
6. Kernel desbloquea el proceso -> READY
```

**No se conecta a Memoria** porque no transfiere datos.

**Analogia real**: `sleep()` en UNIX, timer hardware, operacion de un dispositivo mecanico (ej: mover cabezal de impresora).

**Config**: Solo necesita IP/puerto del Kernel y TIEMPO_UNIDAD_TRABAJO.

---

#### STDIN - Dispositivo de entrada (Teclado)

**Teoria**: Silberschatz (Cap. 13.3) clasifica al teclado como dispositivo de **caracteres** y de **entrada**. Los datos fluyen desde el mundo exterior hacia la memoria del proceso. En un SO real, el driver del teclado maneja interrupciones por cada tecla presionada, las almacena en un buffer del kernel, y cuando el proceso hace `read()`, los datos se copian del buffer del kernel al espacio de memoria del proceso.

**Concepto**: El proceso necesita leer datos del "usuario" y guardarlos en una direccion de su espacio de memoria. La interfaz STDIN:
1. Recibe del Kernel: PID, direccion logica de destino, tamanio a leer
2. Lee los datos (del teclado, de un archivo, etc.)
3. **Escribe esos datos en la memoria del proceso** contactando al modulo Memoria

**Mecanismo**:
```
Instruccion: IO_STDIN_READ <nombre_interfaz> <dir_logica> <tamanio>

1. CPU ejecuta IO_STDIN_READ TECLADO 0x100 64
   -> Proceso quiere leer 64 bytes desde el teclado
   -> Guardarlos a partir de la direccion logica 0x100

2. Kernel bloquea proceso, envia a interfaz "TECLADO":
   (pid, dir_logica=0x100, tamanio=64)

3. Interfaz STDIN:
   a) Lee datos de la entrada (fgets, read, etc.)
   b) Conecta con Memoria y escribe:
      OP_MEM_ESCRIBIR(pid, dir_logica=0x100, buffer=datos_leidos, size=64)
   c) Memoria traduce dir_logica a dir_fisica y almacena los datos

4. Interfaz envia FIN_IO al Kernel
5. Kernel desbloquea proceso -> READY
```

**Se conecta a Memoria** porque necesita escribir datos en el espacio del proceso.

**Flujo de datos**: Mundo exterior -> Interfaz STDIN -> Modulo Memoria -> RAM del proceso

**Analogia real**: `scanf()`, `fgets()`, `read(STDIN_FILENO, ...)` en C. El driver del teclado + buffer del kernel + system call `read()`.

---

#### STDOUT - Dispositivo de salida (Pantalla)

**Teoria**: La pantalla/terminal es un dispositivo de **caracteres** y de **salida**. Los datos fluyen desde la memoria del proceso hacia el mundo exterior. En un SO real, el proceso hace `write()` al file descriptor de stdout, el kernel copia los datos del espacio del proceso al buffer del driver de video, y el driver actualiza el framebuffer o envia los caracteres al terminal.

**Concepto**: El proceso quiere mostrar datos que estan en su memoria. La interfaz STDOUT:
1. Recibe del Kernel: PID, direccion logica de origen, tamanio a leer
2. **Lee los datos de la memoria del proceso** contactando al modulo Memoria
3. Muestra los datos por pantalla (o los imprime en log)

**Mecanismo**:
```
Instruccion: IO_STDOUT_WRITE <nombre_interfaz> <dir_logica> <tamanio>

1. CPU ejecuta IO_STDOUT_WRITE PANTALLA 0x200 32
   -> Proceso quiere mostrar 32 bytes que estan en dir 0x200

2. Kernel bloquea proceso, envia a interfaz "PANTALLA":
   (pid, dir_logica=0x200, tamanio=32)

3. Interfaz STDOUT:
   a) Conecta con Memoria y lee:
      OP_MEM_LEER(pid, dir_logica=0x200, size=32)
   b) Memoria traduce dir_logica, lee de RAM, devuelve los bytes
   c) Interfaz muestra los datos: printf("%s", datos_leidos)

4. Interfaz envia FIN_IO al Kernel
5. Kernel desbloquea proceso -> READY
```

**Se conecta a Memoria** porque necesita leer datos del espacio del proceso.

**Flujo de datos**: RAM del proceso -> Modulo Memoria -> Interfaz STDOUT -> Pantalla

**Analogia real**: `printf()`, `write(STDOUT_FILENO, ...)` en C. El system call `write()` + driver del terminal.

**Nota**: STDIN escribe EN memoria (input -> memoria), STDOUT lee DE memoria (memoria -> output). Es intuitivo si pensas desde el punto de vista del proceso: el proceso "lee" del teclado y "escribe" a la pantalla.

---

#### DIALFS - Dispositivo de almacenamiento (Filesystem)

**Teoria**: Stallings (Cap. 12) y Silberschatz (Cap. 11-14) cubren filesystems extensamente. Un disco es un dispositivo de **bloques** y **bidireccional**. Es el unico dispositivo de I/O que combina:
- **Persistencia**: los datos sobreviven al apagado
- **Acceso aleatorio**: se puede leer/escribir cualquier bloque sin recorrer los anteriores
- **Gran capacidad**: ordenes de magnitud mas que RAM

El filesystem es la capa de software que organiza los bloques raw del disco en una estructura logica de archivos y directorios.

**Concepto de DIALFS**: Es un filesystem simple basado en **asignacion contigua** de bloques con bitmap. Cada archivo ocupa bloques consecutivos en el disco virtual. Soporta 5 operaciones:

**Operacion CREATE**:
```
Instruccion: IO_FS_CREATE <interfaz> <nombre_archivo>

1. CPU ejecuta IO_FS_CREATE DISCO archivo.txt
2. Kernel bloquea proceso, envia a interfaz "DISCO": (pid, nombre="archivo.txt")
3. Interfaz DIALFS:
   a) Crea entrada de metadata: {nombre="archivo.txt", bloque_inicio=0, tamanio=0}
   b) No asigna bloques todavia (archivo vacio)
4. FIN_IO -> Kernel desbloquea -> READY

Analogia real: open(path, O_CREAT), creat(), fopen("w")
Teoria: crear entrada en directorio, asignar inodo (UNIX)
```

**Operacion DELETE**:
```
Instruccion: IO_FS_DELETE <interfaz> <nombre_archivo>

1. CPU ejecuta IO_FS_DELETE DISCO archivo.txt
2. Kernel bloquea, envia a DIALFS
3. Interfaz DIALFS:
   a) Busca metadata del archivo
   b) Libera bloques en bitmap (marca como libres)
   c) Elimina entrada de metadata
4. FIN_IO -> desbloquea

Analogia real: unlink(), remove()
Teoria: liberar bloques de datos, liberar inodo, eliminar entrada del directorio
Nota: en UNIX, unlink solo decrementa el link count. El archivo se borra cuando count=0.
```

**Operacion TRUNCATE**:
```
Instruccion: IO_FS_TRUNCATE <interfaz> <nombre_archivo> <nuevo_tamanio>

1. CPU ejecuta IO_FS_TRUNCATE DISCO archivo.txt 1024
2. Kernel bloquea, envia: (pid, nombre, tamanio=1024)
3. Interfaz DIALFS:
   a) Si crece: busca bloques libres contiguos, extiende el archivo
   b) Si decrece: libera bloques sobrantes en bitmap
   c) Actualiza metadata con nuevo tamanio
4. FIN_IO -> desbloquea

Analogia real: ftruncate(), truncate()
Teoria: modificar tamanio del archivo, reasignar bloques
```

**Operacion WRITE (escritura a archivo)**:
```
Instruccion: IO_FS_WRITE <interfaz> <archivo> <reg_dir> <reg_tam> <ptr_archivo>

Parametros (los valores vienen de registros de CPU):
- reg_dir: registro que contiene la direccion logica de los datos EN MEMORIA del proceso
- reg_tam: registro que contiene cuantos bytes escribir
- ptr_archivo: posicion dentro del archivo donde escribir (file pointer/offset)

1. CPU ejecuta IO_FS_WRITE DISCO archivo.txt SI DI 0
   -> SI contiene la dir logica en memoria (ej: 0x300)
   -> DI contiene el tamanio (ej: 128 bytes)
   -> ptr_archivo = 0 (inicio del archivo)

2. Kernel bloquea, envia: (pid, nombre, dir_mem=0x300, size=128, offset=0)

3. Interfaz DIALFS:
   a) LEE datos de la memoria del proceso:
      OP_MEM_LEER(pid, dir_logica=0x300, size=128)
   b) Memoria traduce, lee de RAM, devuelve 128 bytes
   c) Calcula bloque del archivo: bloque = bloque_inicio + (offset / BLOCK_SIZE)
   d) ESCRIBE los datos en el archivo del filesystem (bloques en disco)
4. FIN_IO -> desbloquea

Flujo de datos: RAM proceso -> Memoria -> DIALFS -> Disco virtual

Analogia real: write(fd, buf, count) con lseek(fd, offset, SEEK_SET)
Teoria: el SO lee del espacio de usuario (copy_from_user en Linux),
        el filesystem calcula los bloques fisicos, el driver escribe al disco
```

**Operacion READ (lectura de archivo)**:
```
Instruccion: IO_FS_READ <interfaz> <archivo> <reg_dir> <reg_tam> <ptr_archivo>

Es la operacion inversa de WRITE:

1. CPU ejecuta IO_FS_READ DISCO archivo.txt SI DI 0
   -> SI contiene dir logica de destino en memoria (ej: 0x400)
   -> DI contiene cuantos bytes leer (ej: 128)
   -> ptr_archivo = 0

2. Kernel bloquea, envia: (pid, nombre, dir_mem=0x400, size=128, offset=0)

3. Interfaz DIALFS:
   a) Calcula bloque: bloque = bloque_inicio + (offset / BLOCK_SIZE)
   b) LEE datos del archivo del filesystem (bloques en disco)
   c) ESCRIBE datos en la memoria del proceso:
      OP_MEM_ESCRIBIR(pid, dir_logica=0x400, buffer=datos_leidos, size=128)
   d) Memoria traduce, escribe en RAM
4. FIN_IO -> desbloquea

Flujo de datos: Disco virtual -> DIALFS -> Memoria -> RAM proceso

Analogia real: read(fd, buf, count) con lseek(fd, offset, SEEK_SET)
Teoria: el filesystem determina bloques fisicos, el driver lee del disco,
        el SO copia al espacio de usuario (copy_to_user en Linux)
```

### 9.8 Resumen de flujo de datos por interfaz

```
GENERICA:  (sin datos)
           Proceso --[bloqueo]--> Kernel --[pedido]--> Interfaz --[usleep]--> FIN_IO

STDIN:     Mundo exterior --> Interfaz --> Memoria --> RAM del proceso
           (input)           (lee input)  (escribe)   (datos guardados)

STDOUT:    RAM del proceso --> Memoria --> Interfaz --> Mundo exterior
           (datos fuente)     (lee)       (muestra)    (output)

DIALFS WRITE: RAM proceso --> Memoria --> Interfaz --> Disco virtual
              (datos fuente)  (lee)       (escribe)    (archivo)

DIALFS READ:  Disco virtual --> Interfaz --> Memoria --> RAM proceso
              (archivo)        (lee)        (escribe)   (datos destino)
```

**Patron comun**: Toda I/O que involucra datos del proceso necesita acceder a la Memoria del proceso. La interfaz nunca accede directamente a la RAM; siempre pasa por el modulo Memoria, que traduce las direcciones logicas del proceso a direcciones fisicas (usando paginacion o segmentacion).

### 9.9 Interfaces como instancias independientes

En un SO real, puede haber multiples dispositivos del mismo tipo (2 discos, 3 terminales). El TP modela esto permitiendo **multiples instancias** del modulo Entrada/Salida, cada una con su nombre y config:

```bash
# Terminal 1: interfaz generica llamada "SLEEP"
./entradasalida SLEEP generica.config

# Terminal 2: otra generica con diferente latencia
./entradasalida TIMER generica_rapida.config

# Terminal 3: stdin llamada "TECLADO"
./entradasalida TECLADO stdin.config

# Terminal 4: stdout llamada "PANTALLA"
./entradasalida PANTALLA stdout.config

# Terminal 5: filesystem llamado "DISCO"
./entradasalida DISCO dialfs.config
```

El Kernel mantiene un registro de interfaces conectadas. Cuando un proceso ejecuta `IO_GEN_SLEEP SLEEP 5`, el Kernel busca la interfaz llamada "SLEEP" y le envia el pedido. Si la interfaz no existe o no esta conectada, el proceso falla.

### 9.10 Relacion con la teoria de Stallings y Silberschatz

| Concepto teorico | Donde se ve en el TP |
|-----------------|---------------------|
| Proceso CPU-bound vs I/O-bound | Un proceso con muchas instrucciones aritmeticas es CPU-bound; uno con muchos IO_GEN_SLEEP es I/O-bound |
| I/O por interrupciones | Cuando DIALFS termina, envia FIN_IO al Kernel (simula interrupcion) |
| Buffering | Interfaz almacena datos en buffer local antes de enviar a Memoria |
| Device driver | El modulo Entrada/Salida actua como driver: recibe operacion generica, la ejecuta segun el tipo de dispositivo |
| Device controller | Simulado por la logica interna de cada interfaz (GENERICA hace sleep, STDIN lee teclado, etc.) |
| I/O scheduling | Kernel decide que pedido de I/O enviar a cada interfaz. Las interfaces procesan un pedido a la vez (FIFO) |
| DMA | DIALFS transfiere datos entre disco y Memoria sin pasar por CPU (CPU solo inicia la operacion) |
| Blocking I/O | Toda I/O en el TP es bloqueante: el proceso se bloquea hasta que la interfaz completa la operacion |
| Non-blocking I/O | No implementado, pero en un SO real: select(), poll(), epoll(), async I/O |
| Spooling | No implementado, pero si hubiera una interfaz "IMPRESORA", multiples procesos podrian encolar trabajos |

---

## 10. COMUNICACION ENTRE PROCESOS (IPC)

### 10.1 Mecanismos

| Mecanismo | Tipo | Descripcion |
|-----------|------|-------------|
| **Shared Memory** | Directo | Procesos comparten region de memoria. Rapido pero requiere sincronizacion. |
| **Message Passing** | Indirecto | Envio/recepcion de mensajes. Mas lento pero mas seguro. |
| **Pipes** | Unidireccional | FIFO entre procesos (anonimos o nombrados) |
| **Sockets** | Bidireccional | Comunicacion local o en red. TCP (confiable) o UDP (no confiable). |
| **Signals** | Asincronico | Notificaciones del SO a procesos (SIGKILL, SIGTERM, etc.) |
| **RPC** | Remoto | Llamadas a procedimientos en otra maquina/proceso |

### 10.2 Sockets TCP

```
Servidor:                    Cliente:
socket()                     socket()
bind(puerto)                 connect(ip, puerto)
listen()                         |
accept() <----- conexion --------+
recv()/send() <-> send()/recv()
close()                      close()
```

### 10.3 En el TP

- **Sockets TCP** para toda la comunicacion entre los 5 modulos
- **Protocolo de mensajes**: Cada paquete tiene `op_code` (enum) + payload serializado
- **Op_codes** agrupados por rango: 200s CPU, 300s Memoria, 400s I/O
- **Serializacion manual**: Los structs se empaquetan campo por campo (`paquete_write_uint32`, `paquete_write_string`, etc.)
- **Handshake**: Al conectarse, los modulos intercambian un paquete de identificacion

---

## 11. PROTECCION Y SEGURIDAD

### Modo dual

| Modo | Acceso | Quien ejecuta |
|------|--------|---------------|
| **Kernel mode** | Total: hardware, instrucciones privilegiadas, toda la memoria | SO (kernel) |
| **User mode** | Restringido: solo su espacio de memoria, sin instrucciones privilegiadas | Procesos de usuario |

**System call**: Mecanismo para que un proceso en user mode solicite un servicio al kernel (trap/interrupt que cambia a kernel mode).

### Proteccion de memoria

- **Base + Limite**: Cada proceso tiene registros base y limite. Acceso fuera de rango genera fault.
- **Paginacion**: Bits de proteccion por pagina (read-only, read-write, execute-only)
- **Segmentacion**: Limite por segmento, segmentation fault si offset >= limite

### En el TP

- Segmentation fault en segmentacion cuando `offset >= limite`
- Page fault en paginacion cuando pagina no esta presente
- El adapter de memoria valida todos los accesos antes de ejecutarlos

---

## 12. CONCEPTOS DE C PARA IMPLEMENTACION

### Memoria dinamica

```c
void* ptr = malloc(tamanio);    // Reserva memoria en heap
free(ptr);                       // Libera memoria
// SIEMPRE liberar lo que se aloca. Un free faltante = memory leak.
// NUNCA usar memoria despues de free = use-after-free (undefined behavior).
```

### Punteros

```c
int x = 42;
int* p = &x;       // p apunta a x
*p = 100;           // x ahora vale 100

void* generico;     // Puntero generico (sin tipo). Hay que castear para usar.
(int*)generico      // Cast a puntero a int

// Aritmetica: p + 1 avanza sizeof(*p) bytes
char* base = (char*)memoria;
base[offset] // accede a memoria + offset
```

### Structs

```c
typedef struct {
    uint32_t pid;
    uint32_t pc;
    int estado;
} t_pcb;

t_pcb* pcb = malloc(sizeof(t_pcb));
pcb->pid = 1;      // Acceso via puntero
(*pcb).pc = 0;      // Equivalente
```

### pthreads

```c
#include <pthread.h>

void* funcion_hilo(void* arg) {
    // ... trabajo del hilo ...
    return NULL;
}

pthread_t hilo;
pthread_create(&hilo, NULL, funcion_hilo, argumento);
pthread_join(hilo, NULL);    // Espera a que termine
pthread_detach(hilo);        // No esperar, se limpia solo
```

### Sincronizacion

```c
// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_lock(&mutex);
// seccion critica
pthread_mutex_unlock(&mutex);

// Semaforo
#include <semaphore.h>
sem_t sem;
sem_init(&sem, 0, valor_inicial);
sem_wait(&sem);   // P (decrementa o bloquea)
sem_post(&sem);   // V (incrementa y despierta)

// Condition Variable
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_wait(&cond, &mutex);     // Espera (libera mutex mientras espera)
pthread_cond_signal(&cond);           // Despierta un hilo
pthread_cond_broadcast(&cond);        // Despierta todos
```

### Sockets (TCP)

```c
#include <sys/socket.h>
#include <netinet/in.h>

// Servidor
int server = socket(AF_INET, SOCK_STREAM, 0);
bind(server, (struct sockaddr*)&addr, sizeof(addr));
listen(server, SOMAXCONN);
int cliente = accept(server, NULL, NULL);
recv(cliente, buffer, size, 0);
send(cliente, data, size, 0);

// Cliente
int sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, (struct sockaddr*)&addr, sizeof(addr));
send(sock, data, size, 0);
recv(sock, buffer, size, 0);
```

### so-commons-library (UTN)

```c
// Listas
t_list* lista = list_create();
list_add(lista, elemento);
void* elem = list_get(lista, index);
void* elem = list_remove(lista, index);
int tam = list_size(lista);

// Diccionarios
t_dictionary* dict = dictionary_create();
dictionary_put(dict, "clave", valor);
void* val = dictionary_get(dict, "clave");
bool exists = dictionary_has_key(dict, "clave");

// Config
t_config* cfg = config_create("archivo.config");
char* str = config_get_string_value(cfg, "CLAVE");
int num = config_get_int_value(cfg, "CLAVE");

// Logs
t_log* log = log_create("archivo.log", "MODULO", true, LOG_LEVEL_INFO);
log_info(log, "Mensaje %d", valor);
log_error(log, "Error: %s", descripcion);

// Bitarray
t_bitarray* bits = bitarray_create_with_mode(data, size, LSB_FIRST);
bitarray_set_bit(bits, index);     // Poner en 1
bitarray_clean_bit(bits, index);   // Poner en 0
bool val = bitarray_test_bit(bits, index);  // Leer

// Strings
char* str = string_itoa(42);          // int -> string
char** arr = string_split(linea, " "); // Split por separador
```
