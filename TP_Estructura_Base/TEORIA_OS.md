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

### 9.1 Conceptos

**Tipos de dispositivos:**
- **Por bloques**: Acceso aleatorio a bloques de datos (disco, SSD)
- **Por caracteres**: Stream de bytes secuencial (teclado, mouse, puerto serial)

**Tecnicas de I/O:**

| Tecnica | Mecanismo | CPU |
|---------|-----------|-----|
| **Programmed I/O** | CPU hace polling (busy waiting) | CPU ocupada 100% |
| **Interrupt-driven** | Dispositivo interrumpe a CPU al completar | CPU libre mientras espera |
| **DMA** | Controlador DMA transfiere datos directo a RAM | CPU solo inicia y recibe interrupcion al final |

**Buffering**: Almacenar datos temporalmente durante transferencia
**Caching**: Guardar copia de datos frecuentes en memoria rapida
**Spooling**: Cola de trabajos para dispositivos que no soportan acceso concurrente (impresora)

### 9.2 En el TP

4 tipos de interfaz simulando diferentes dispositivos:

| Interfaz | Dispositivo simulado | Conecta a Memoria | Instruccion |
|----------|---------------------|-------------------|-------------|
| **GENERICA** | Device con latencia | No | `IO_GEN_SLEEP INTERFAZ UNIDADES` |
| **STDIN** | Teclado | Si (escribe) | `IO_STDIN_READ INTERFAZ DIR TAM` |
| **STDOUT** | Pantalla | Si (lee) | `IO_STDOUT_WRITE INTERFAZ DIR TAM` |
| **DIALFS** | Disco/Filesystem | Si (lee/escribe) | `IO_FS_*` |

Cada interfaz es una instancia independiente del modulo entradasalida, configurada con su propio `.config`.

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
