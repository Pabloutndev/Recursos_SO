# Descripcion de Modulos - TP Sistema Operativo

## Arquitectura General

El sistema simula un SO con 5 modulos independientes comunicados por sockets TCP. Cada modulo es un proceso separado que se levanta con su propio archivo `.config`.

```
Consola <--TCP--> Kernel <--TCP--> CPU
                    |                |
                    |                v
                    +-----------> Memoria
                    |
                    v
               Entrada/Salida
```

---

## Kernel

Nucleo del sistema. Administra procesos, planificacion y recursos.

- Recibe comandos de la Consola (RUN, KILL, PS, PAUSE, START, etc.)
- Crea y destruye procesos (PCB con pid, pc, registros, estado, quantum)
- Planificacion de largo plazo: mueve procesos de NEW a READY respetando el grado de multiprogramacion (`sem_mp`)
- Planificacion de corto plazo: selecciona el proximo proceso a ejecutar segun el algoritmo configurado
- Algoritmos soportados: FIFO, RR (Round Robin), VRR (Virtual Round Robin), HRRN
- Envia procesos a CPU via socket dispatch y recibe el contexto actualizado
- Envia interrupciones a CPU via socket interrupt (ej: fin de quantum)
- Administra recursos con WAIT/SIGNAL
- Maneja bloqueo por I/O: envia solicitudes a las interfaces de Entrada/Salida

**Config:** `kernel.config` (puertos, algoritmo, quantum, recursos, grado multiprogramacion)

---

## CPU

Simula el procesador. Ejecuta instrucciones de un proceso recibido del Kernel.

- Recibe un proceso (contexto) por socket dispatch del Kernel
- Ejecuta el ciclo Fetch-Decode-Execute en loop hasta que ocurra un evento (EXIT, IO, quantum)
- Fetch: solicita la instruccion a Memoria segun el PC
- Decode: parsea la instruccion de texto en opcode + parametros
- Execute: ejecuta la operacion y actualiza registros/PC
- MMU: traduce direcciones logicas a fisicas usando tabla de paginas (consultando a Memoria)
- TLB: cache de traducciones con algoritmos FIFO o LRU
- Escucha interrupciones del Kernel por socket interrupt
- Al terminar, devuelve el contexto actualizado al Kernel con el motivo (EXIT, IO, QUANTUM, SEGFAULT)

**Instrucciones soportadas:**

| Instruccion | Formato | Descripcion |
|-------------|---------|-------------|
| SET | `SET REG VALOR` | Asigna un valor inmediato a un registro |
| SUM | `SUM REG1 REG2` | REG1 = REG1 + REG2 |
| SUB | `SUB REG1 REG2` | REG1 = REG1 - REG2 |
| JNZ | `JNZ REG INST` | Si REG != 0, salta al PC indicado por INST |
| MOV_IN | `MOV_IN REG DIR_LOG` | Lee de memoria (dir logica) y guarda en registro |
| MOV_OUT | `MOV_OUT DIR_LOG REG` | Escribe el valor del registro en memoria (dir logica) |
| RESIZE | `RESIZE TAMANIO` | Solicita a Memoria redimensionar el proceso |
| COPY_STRING | `COPY_STRING TAMANIO` | Copia un string de memoria (SI -> DI) |
| WAIT | `WAIT RECURSO` | Solicita un recurso al Kernel |
| SIGNAL | `SIGNAL RECURSO` | Libera un recurso al Kernel |
| IO_GEN_SLEEP | `IO_GEN_SLEEP INTERFAZ UNIDADES` | Bloquea el proceso N unidades de tiempo |
| IO_STDIN_READ | `IO_STDIN_READ INTERFAZ DIR_LOG TAMANIO` | Lee input y lo escribe en memoria |
| IO_STDOUT_WRITE | `IO_STDOUT_WRITE INTERFAZ DIR_LOG TAMANIO` | Lee de memoria y lo muestra por pantalla |
| IO_FS_CREATE | `IO_FS_CREATE INTERFAZ ARCHIVO` | Crea un archivo en el filesystem |
| IO_FS_DELETE | `IO_FS_DELETE INTERFAZ ARCHIVO` | Elimina un archivo del filesystem |
| IO_FS_TRUNCATE | `IO_FS_TRUNCATE INTERFAZ ARCHIVO TAMANIO` | Redimensiona un archivo |
| IO_FS_WRITE | `IO_FS_WRITE INTERFAZ ARCHIVO REG_DIR REG_TAM PTR_ARCHIVO` | Escribe en archivo desde memoria |
| IO_FS_READ | `IO_FS_READ INTERFAZ ARCHIVO REG_DIR REG_TAM PTR_ARCHIVO` | Lee de archivo hacia memoria |
| EXIT | `EXIT` | Finaliza el proceso |

**Registros:** AX, BX, CX, DX (8 bits), EAX, EBX, ECX, EDX (32 bits), SI, DI (32 bits), PC (32 bits)

**Config:** `cpu.config` (conexion a memoria, puertos dispatch/interrupt, TLB, tamanio pagina)

---

## Memoria

Simula la memoria RAM. Soporta dos esquemas de gestion configurable: **paginacion** y **segmentacion**.

- Capa de abstraccion `esquema_memoria` (`esquema_memoria.h/c`) que permite cambiar el esquema via config sin modificar el resto del sistema
- Almacena instrucciones y datos de los procesos
- RAM simulada como bloque contiguo de `TAM_MEMORIA` bytes
- Traduce direcciones logicas a fisicas cuando CPU lo solicita
- Atiende solicitudes de CPU (fetch instruccion, lectura/escritura de datos, traduccion de direcciones)
- Atiende solicitudes de Kernel (crear/destruir proceso)
- Atiende solicitudes de Entrada/Salida (lectura/escritura directa a memoria)

**Esquema PAGINACION** (`paginas.c`):
- Tabla de paginas por proceso (dictionary indexado por PID)
- Frames administrados con bitmap (bitarray)
- Bits de control: presente, modificado, uso
- Algoritmo de reemplazo Clock con puntero circular persistente
- Swap a disco: cuando no hay frames libres, desaloja una pagina a archivo de swap

**Esquema SEGMENTACION** (`segmentacion.c`):
- Un segmento contiguo por proceso (base + limite)
- Free list con algoritmo First-Fit para asignacion
- Merge de bloques adyacentes al liberar memoria
- Resize con reubicacion automatica si no hay espacio adyacente
- Segmentation fault si el offset excede el limite del segmento

**Config:** `memoria.config`
```
PUERTO_ESCUCHA=8002
TAM_MEMORIA=4096
TAM_PAGINA=256
ESQUEMA_MEMORIA=PAGINACION    # PAGINACION o SEGMENTACION
ALGORITMO_REEMPLAZO=CLOCK     # solo aplica en paginacion
RETARDO_RESPUESTA=100
```

---

## Entrada/Salida

Simula dispositivos de I/O. Se puede instanciar multiples veces, una por cada interfaz.

Se conecta al Kernel (para recibir solicitudes) y opcionalmente a Memoria (para leer/escribir datos). Se lanza indicando el nombre de la interfaz y el archivo config como argumentos.

### Tipos de interfaz:

**GENERICA**
- Simula un dispositivo simple que solo bloquea al proceso un tiempo determinado
- Instruccion: `IO_GEN_SLEEP INTERFAZ UNIDADES`
- El tiempo de bloqueo = unidades * TIEMPO_UNIDAD_TRABAJO (ms) del config
- No se conecta a Memoria
- Uso: `./entradasalida SLEEP generica.config`

**STDIN**
- Simula entrada por teclado
- Instruccion: `IO_STDIN_READ INTERFAZ DIR_LOGICA TAMANIO`
- Lee texto del teclado (o fuente de entrada) y lo escribe en la direccion de memoria del proceso
- Se conecta a Memoria para escribir los datos leidos
- Uso: `./entradasalida TECLADO stdin.config`

**STDOUT**
- Simula salida por pantalla
- Instruccion: `IO_STDOUT_WRITE INTERFAZ DIR_LOGICA TAMANIO`
- Lee datos de la direccion de memoria del proceso y los muestra por pantalla
- Se conecta a Memoria para leer los datos
- Uso: `./entradasalida PANTALLA stdout.config`

**DIALFS**
- Simula un filesystem basado en bloques
- Instrucciones: IO_FS_CREATE, IO_FS_DELETE, IO_FS_TRUNCATE, IO_FS_WRITE, IO_FS_READ
- Administra archivos sobre un espacio de bloques contiguos con bitmap de bloques libres
- Se conecta a Memoria para leer/escribir datos de los procesos
- Config adicional: PATH_BASE_DIALFS, BLOCK_SIZE, BLOCK_COUNT
- Uso: `./entradasalida DISCO dialfs.config`

---

## Consola

Cliente remoto que se conecta al Kernel para enviar comandos interactivamente.

**Comandos disponibles:**

| Comando | Formato | Descripcion |
|---------|---------|-------------|
| RUN | `RUN <archivo>` | Ejecuta un proceso a partir de un archivo de instrucciones |
| KILL | `KILL <pid>` | Mata un proceso por su PID |
| PS | `PS` | Muestra el estado de todos los procesos (NEW, READY, EXEC, BLOCKED, EXIT) |
| START | `START` | Inicia o reanuda la planificacion |
| PAUSE | `PAUSE` | Pausa la planificacion (los procesos en EXEC siguen hasta el proximo evento) |
| ALGORITMO | `ALGORITMO <alg>` | Cambia el algoritmo de planificacion (FIFO, RR, VRR, HRRN) |
| DESALOJAR | `DESALOJAR <pid>` | Fuerza el desalojo de un proceso en ejecucion |
| HELP | `HELP` | Muestra la lista de comandos disponibles |
| EXIT | `EXIT` | Cierra la consola |

**Config:** `consola.config` (IP y puerto del Kernel)

---

## Utils (Biblioteca compartida)

No es un modulo ejecutable. Es una biblioteca `.so` compartida que usan todos los modulos.

- Protocolo de paquetes: crear, escribir datos, leer datos, destruir
- Op_codes: enum centralizado con codigos de operacion agrupados por rango (200s CPU, 300s Memoria, 400s IO)
- Serializacion/deserializacion de estructuras para envio por socket
- Funciones de conexion: crear socket, escuchar, aceptar, handshake
- Utilidades: lectura de archivos de instrucciones, funciones compartidas

---

## Orden de arranque

1. **Memoria** (servidor, escucha en puerto 8002)
2. **CPU** (se conecta a Memoria, escucha dispatch:8006 e interrupt:8007)
3. **Kernel** (se conecta a Memoria y CPU, escucha consola:8010 e IO:8003)
4. **Entrada/Salida** (se conecta a Kernel y opcionalmente a Memoria)
5. **Consola** (se conecta a Kernel)
