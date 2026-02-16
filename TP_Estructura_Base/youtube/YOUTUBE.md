# Curso de Sistemas Operativos - Estructura de Videos

Guia para armar un curso en video explicando cada concepto de SO y su implementacion en el TP.
Cada video es autocontenido pero sigue un orden progresivo.

---

## BLOQUE 1: Fundamentos y Arquitectura (3 videos)

### Video 1 - Introduccion al Sistema Operativo (15-20 min)
**Concepto:** Que es un SO, por que existe, que problemas resuelve.
**Codigo:**
- Arquitectura general del TP: 5 modulos (Kernel, CPU, Memoria, IO, Consola)
- Diagrama de comunicacion por sockets TCP
- `PROYECTO.md` → flujo de vida de un proceso (NEW → READY → EXEC → EXIT)
- Recorrido rapido del repositorio: carpetas, Makefiles, como compilar y ejecutar

### Video 2 - Protocolo de Comunicacion (10-15 min)
**Concepto:** IPC (Inter-Process Communication), sockets TCP, serializacion.
**Codigo:**
- `utils/src/protocolo/op_code.h` → rangos de opcodes (genericos, CPU, Memoria, IO)
- `utils/src/paquete/paquete.c` → crear paquete, escribir datos, enviar/recibir
- `utils/src/serializacion/serializacion.c` → como se serializa un contexto CPU
- `utils/src/conexion/conexion.c` → `crear_conexion()`, `iniciar_servidor()`, handshake
- Demo: flujo de un paquete desde Consola hasta Kernel

### Video 3 - La Consola: Interfaz de Usuario (5-10 min)
**Concepto:** Interfaz humano-maquina, REPL (Read-Eval-Print Loop).
**Codigo:**
- `consola/src/consola.c` → loop con readline, parseo de comandos
- `consola/src/helpers/consola_helpers.c` → tabla de comandos
- `consola/src/adaptadores/consola_kernel_adapter.c` → envio de paquetes al Kernel
- Comandos: RUN, KILL, PS, ALGORITMO, START, PAUSE

---

## BLOQUE 2: Gestion de Procesos (4 videos)

### Video 4 - El PCB: Bloque de Control de Proceso (10-15 min)
**Concepto:** Que informacion necesita el SO para administrar un proceso.
**Codigo:**
- `kernel/src/pcb/pcb.h` → struct `t_pcb`: pid, estado, pc, registros, quantum, prioridad, path, `recursos_adquiridos`
- `kernel/src/pcb/pcb.c` → `pcb_crear()`, `pcb_destruir()`
- Estados: NEW, READY, EXEC, BLOCK, EXIT
- Transiciones de estado (diagrama)

### Video 5 - Creacion y Destruccion de Procesos (10-15 min)
**Concepto:** Ciclo de vida completo. Como nace y muere un proceso.
**Codigo:**
- `kernel/src/peticiones/proceso.c` → `ejecutar_proceso()`: validar path, crear PCB, encolar en NEW
- `kernel/src/peticiones/ruta_procesos.c` → validacion de archivos de instrucciones
- `kernel/src/planificacion/planificacion.c` → `planificacion_finalizar_proceso()` (KILL)
- `kernel/src/conexiones/consola_handler.c` → como llega un RUN desde la consola
- `memoria/src/gestion/memoria_core.c` → `memoria_crear_proceso()`: cargar instrucciones en RAM
- Demo: test 1 (ciclo basico NEW → READY → EXEC → EXIT)

### Video 6 - Planificador de Largo Plazo (10-15 min)
**Concepto:** Grado de multiprogramacion, control de admision.
**Codigo:**
- `kernel/src/planificacion/largo_plazo.c` → hilo que mueve de NEW a READY
- Semaforos: `sem_hay_new` (hay procesos nuevos), `sem_mp` (slots de multiprogramacion)
- `planificacion_check_pause()` → mecanismo PAUSE/START con condition variable
- Interaccion con Memoria: `solicitar_creacion_proceso_memoria()`
- Demo: grado multiprogramacion=2, 3 procesos en cola

### Video 7 - Planificador de Corto Plazo (15-20 min)
**Concepto:** Scheduling: quien ejecuta y cuando.
**Codigo:**
- `kernel/src/planificacion/corto_plazo.c` → hilo del scheduler
- Flujo: `sem_wait(sem_hay_ready)` → `proximoAEjecutar()` → dispatch a CPU → `atender_dispatch_cpu()` (bloqueante)
- Timer de quantum: hilo separado con `usleep()`, verifica si sigue en EXEC
- Function pointer `proximoAEjecutar` para cambio dinamico de algoritmo
- Demo: test 4 (quantum RR) mostrando desalojo

---

## BLOQUE 3: Algoritmos de Planificacion (3 videos)

### Video 8 - FIFO y Round Robin (15-20 min)
**Concepto:** Algoritmos basicos. FCFS vs tiempo compartido.
**Codigo:**
- `kernel/src/planificacion/algoritmo.c`:
  - `algoritmo_obtener_fifo()` → pop del primero en cola
  - `algoritmo_obtener_rr()` → igual que FIFO pero con quantum
- `corto_plazo.c` → logica de quantum: `timer_quantum()`, `usleep(quantum * 1000)`
- Comparacion: test 5 (FIFO sin desalojo) vs test 4 (RR con desalojo)
- Pros y contras de cada uno

### Video 9 - VRR, HRRN y Prioridades (15-20 min)
**Concepto:** Algoritmos avanzados. Fairness, starvation, prioridades.
**Codigo:**
- `algoritmo.c`:
  - `algoritmo_obtener_vrr()` → quantum variable, procesos con IO reciben bonus
  - `algoritmo_obtener_hrrn()` → Highest Response Ratio Next, calcula ratio (espera + servicio) / servicio
  - `algoritmo_obtener_prioridad()` → selecciona menor valor de `pcb->prioridad`
- `planificacion.c` → `set_algoritmo()`: cambio dinamico con function pointer
- `consola_handler.c` → comando ALGORITMO
- Demo: test 10 (prioridades con PAUSE/START)

### Video 10 - Cambio de Contexto (10 min)
**Concepto:** Que pasa cuando un proceso deja la CPU. Context switch.
**Codigo:**
- `kernel/src/peticiones/dispatch.c` → `enviar_proceso_a_cpu()`: PCB → contexto → socket
- `kernel/src/peticiones/interrupciones.c` → `manejar_interrupcion()`: segun motivo, mueve a READY/BLOCKED/EXIT
- `kernel/src/peticiones/interrupciones_handlers.c` → handlers especificos
- `cpu/src/adaptadores/cpu_dispatch_handler.c` → recibe contexto, ejecuta, devuelve resultado

---

## BLOQUE 4: La CPU (3 videos)

### Video 11 - Ciclo Fetch-Decode-Execute (15-20 min)
**Concepto:** Como ejecuta instrucciones un procesador.
**Codigo:**
- `cpu/src/ciclo_instruccion/ciclo.c` → `ciclo_instruccion_ejecutar()`: while loop con fetch/decode/execute
- Fetch: pide instruccion a Memoria via socket (`cpu_memoria_adapter.c`)
- `cpu/src/ciclo_instruccion/decode.c` → parsea string "SET AX 10" a instruccion estructurada
- `cpu/src/instrucciones/operaciones.c` → SET, SUM, SUB, JNZ
- `cpu/src/instrucciones/operaciones_mem.c` → MOV_IN, MOV_OUT, RESIZE
- Demo: seguir instruccion por instruccion con logs

### Video 12 - Registros e Instrucciones (10-15 min)
**Concepto:** Registros del procesador, ISA (Instruction Set Architecture).
**Codigo:**
- `cpu/src/registros/registros.c` → lectura/escritura generica por nombre
- Registros: AX-DX (8 bits), EAX-EDX + SI + DI (32 bits), PC
- Tabla completa de instrucciones con formatos
- WAIT/SIGNAL: como la CPU delega al Kernel

### Video 13 - Interrupciones y Desalojo (10-15 min)
**Concepto:** Interrupciones de hardware vs software. Mecanismo de interrupcion.
**Codigo:**
- `cpu/src/interrupciones/interrupciones.c` → flag `volatile bool`, `interrupcion_pendiente()`
- `cpu/src/server/cpu_server.c` → hilo interrupt: recibe del Kernel, setea flag
- El ciclo chequea el flag ANTES de cada fetch
- Motivos de desalojo: EXIT, IO, QUANTUM, SEGFAULT
- `cpu/src/adaptadores/cpu_dispatch_handler.c` → decision del opcode de respuesta

---

## BLOQUE 5: Gestion de Memoria (4 videos)

### Video 14 - Memoria: Conceptos y Arquitectura (10-15 min)
**Concepto:** Memoria virtual, espacio de direcciones, por que abstraer la memoria.
**Codigo:**
- `memoria/src/gestion/memoria_core.c` → crear/destruir proceso, fetch instruccion
- `memoria/src/gestion/memoria_ram.c` → RAM como array de bytes, leer/escribir
- `memoria/src/gestion/esquema_memoria.c` → capa de abstraccion: paginacion vs segmentacion
- `memoria/src/adaptadores/memoria_adapter.c` → handlers de requests (init, fin, fetch, read, write)

### Video 15 - Paginacion (20-25 min)
**Concepto:** Paginas, frames, tabla de paginas, traduccion de direcciones.
**Codigo:**
- `memoria/src/gestion/paginas.c` → tabla de paginas por proceso, crear/destruir/resize
- `memoria/src/gestion/paginas_internal.h` → entrada: frame, presente, dirty, use
- `memoria/src/gestion/frames.c` → bitmap de frames, asignar/liberar
- Traduccion: pagina = dir_logica / tam_pagina, offset = dir_logica % tam_pagina
- Demo: MOV_OUT + MOV_IN (test 6)

### Video 16 - MMU y TLB en la CPU (15-20 min)
**Concepto:** MMU (Memory Management Unit), TLB (Translation Lookaside Buffer), cache de traducciones.
**Codigo:**
- `cpu/src/mmu/mmu.c` → `mmu_traducir()`: consulta TLB, si miss pide a Memoria
- `cpu/src/tlb/tlb.c` → cache de pagina→frame, algoritmos FIFO y LRU
- `tlb_buscar()`, `tlb_agregar()`, `tlb_limpiar_proceso()`
- Flujo completo: CPU quiere leer dir logica → MMU → TLB hit/miss → Memoria → dir fisica

### Video 17 - Reemplazo de Paginas y Swap (15-20 min)
**Concepto:** Que pasa cuando no hay frames libres. Page fault, swap.
**Codigo:**
- `memoria/src/gestion/reemplazo.c` → algoritmo Clock: puntero circular, bit de uso
- `memoria/src/swap/swap.c` → escribir pagina a disco, leer de disco
- Page fault: pagina no presente → traer de swap → si no hay frame → desalojar victima
- Bits de control: presente (en RAM), dirty (modificada), use (para Clock)

---

## BLOQUE 6: Segmentacion (1 video)

### Video 18 - Segmentacion: Alternativa a Paginacion (15 min)
**Concepto:** Segmentos contiguos, base + limite, fragmentacion externa.
**Codigo:**
- `memoria/src/gestion/segmentacion.c` → un segmento por proceso
- Free list con First-Fit, merge de bloques adyacentes
- Resize con reubicacion automatica
- Segmentation fault: offset >= limite
- Comparacion con paginacion: pros (simple, no fragmentacion interna) vs contras (fragmentacion externa)

---

## BLOQUE 7: Sincronizacion y Recursos (3 videos)

### Video 19 - Recursos: WAIT y SIGNAL (15-20 min)
**Concepto:** Semaforos, seccion critica, exclusion mutua.
**Codigo:**
- `kernel/src/peticiones/recursos.c` → `recurso_wait()`, `recurso_signal()`
- Estructura `t_recurso`: instancias (semaforo), cola_bloqueados, mutex
- Cuando instancias > 0: adquirir. Cuando = 0: bloquear proceso
- Tracking: `pcb->recursos_adquiridos` → lista de recursos que tiene cada proceso
- `kernel.config` → RECURSOS=[RA,RB,RC], INSTANCIAS_RECURSOS=[1,2,1]
- Demo: test 5 (dos procesos comparten recurso RA con 1 instancia)

### Video 20 - Deteccion de Deadlock: Grafo de Espera (15-20 min)
**Concepto:** Deadlock (abrazo mortal), condiciones necesarias, deteccion por ciclos.
**Codigo:**
- `kernel/src/deteccion_deadlock/grafo_espera.c`:
  - Construir grafo: para cada proceso bloqueado, buscar quien tiene el recurso que espera
  - Arista: proceso_bloqueado → proceso_que_tiene_recurso
  - DFS para detectar ciclos (`dfs_ciclo()`)
  - Reconstruccion del ciclo para logging
- `interrupciones_handlers.c` → se llama despues de `recurso_wait()` cuando bloquea
- Demo: test 11 (deadlock_a.txt + deadlock_b.txt → ciclo A↔B)

### Video 21 - Deteccion de Deadlock: Algoritmo del Banquero (15-20 min)
**Concepto:** Estado seguro vs inseguro, prevencion vs deteccion.
**Codigo:**
- `kernel/src/deteccion_deadlock/banquero.c`:
  - Recopilar Available (instancias libres), Allocation (recursos por proceso), Need (que falta)
  - Simular: marcar procesos como "pueden terminar" si Need <= Available
  - Liberar sus recursos → repetir hasta que todos terminen o ninguno pueda
  - Si alguno no puede terminar → estado inseguro → posible deadlock
- Comparacion: Grafo (detecta deadlock actual) vs Banquero (detecta estado inseguro)
- Las 4 condiciones de Coffman: exclusion mutua, hold-and-wait, no preemption, espera circular

---

## BLOQUE 8: Entrada/Salida (2 videos)

### Video 22 - IO Generica, STDIN y STDOUT (15 min)
**Concepto:** Dispositivos de IO, buffering, blocking IO.
**Codigo:**
- `entradasalida/src/core/io_main.c` → conexion al Kernel, registro de interfaz, loop principal
- `entradasalida/src/adaptadores/io_adapter.c`:
  - `handle_sleep()` → simula IO con `usleep()`
  - `handle_stdin_read()` → lee teclado, escribe en Memoria del proceso
  - `handle_stdout_write()` → lee de Memoria, muestra por pantalla
- Kernel side: `kernel_sleep()`, `kernel_stdin_read()`, `kernel_stdout_write()`
- Demo: test 2 (IO bloqueante) y test 3 (IO multiple)

### Video 23 - DialFS: Filesystem (15-20 min)
**Concepto:** Sistema de archivos, bloques, metadata, bitmap.
**Codigo:**
- `entradasalida/src/interfaces/dialfs.c` → filesystem sobre archivo de datos
- `dialfs_bitmap.c` → bitmap de bloques libres, asignar/liberar contiguos
- `dialfs_metadata.c` → FCB (File Control Block): nombre, bloque inicio, tamanio
- Operaciones: CREATE, DELETE, TRUNCATE, WRITE, READ
- Flujo: CPU pide IO_FS_WRITE → Kernel envia a IO → IO lee de Memoria → escribe en archivo

---

## BLOQUE 9: Testing y Cierre (2 videos)

### Video 24 - Test Suite: Como Verificar el SO (10-15 min)
**Concepto:** Testing basado en logs, automatizacion, verificacion de comportamiento.
**Codigo:**
- `tests/run_all.sh` → 11 tests automatizados
- `tests/start_modules.sh` → compilacion y levantamiento de modulos
- Patrones de verificacion: `wait_for_log`, `wait_for_log_count`, `mark_log`
- Recorrido de cada test: que verifica y por que
- Scripts de proceso en `memoria/procesos/`: test1.txt, test_io.txt, infinito.txt, deadlock_a/b.txt

### Video 25 - Recapitulacion: El SO Completo (10-15 min)
**Concepto:** Como encajan todas las piezas.
- Recorrido end-to-end: desde `RUN test1.txt` hasta `EXEC -> EXIT`
- Diagrama de secuencia completo con todos los modulos
- Patrones de diseno usados: semaforos, mutex, condition variables, function pointers
- Reflexion: que simplificamos vs un SO real (Linux)
- Posibles extensiones: scheduling con aging, demand paging, journaling FS

---

## Resumen de Videos

| # | Titulo | Duracion | Bloque |
|---|--------|----------|--------|
| 1 | Introduccion al SO | 15-20 min | Fundamentos |
| 2 | Protocolo de Comunicacion | 10-15 min | Fundamentos |
| 3 | La Consola | 5-10 min | Fundamentos |
| 4 | El PCB | 10-15 min | Procesos |
| 5 | Creacion y Destruccion | 10-15 min | Procesos |
| 6 | Planificador Largo Plazo | 10-15 min | Procesos |
| 7 | Planificador Corto Plazo | 15-20 min | Procesos |
| 8 | FIFO y Round Robin | 15-20 min | Algoritmos |
| 9 | VRR, HRRN y Prioridades | 15-20 min | Algoritmos |
| 10 | Cambio de Contexto | 10 min | Algoritmos |
| 11 | Ciclo Fetch-Decode-Execute | 15-20 min | CPU |
| 12 | Registros e Instrucciones | 10-15 min | CPU |
| 13 | Interrupciones y Desalojo | 10-15 min | CPU |
| 14 | Memoria: Arquitectura | 10-15 min | Memoria |
| 15 | Paginacion | 20-25 min | Memoria |
| 16 | MMU y TLB | 15-20 min | Memoria |
| 17 | Reemplazo y Swap | 15-20 min | Memoria |
| 18 | Segmentacion | 15 min | Segmentacion |
| 19 | Recursos: WAIT/SIGNAL | 15-20 min | Sincronizacion |
| 20 | Deadlock: Grafo de Espera | 15-20 min | Sincronizacion |
| 21 | Deadlock: Banquero | 15-20 min | Sincronizacion |
| 22 | IO Generica, STDIN, STDOUT | 15 min | IO |
| 23 | DialFS: Filesystem | 15-20 min | IO |
| 24 | Test Suite | 10-15 min | Testing |
| 25 | Recapitulacion | 10-15 min | Cierre |

**Duracion total estimada: ~6 horas de contenido**

---

## Tips para Grabar

- Usar split screen: teoria (slides/diagrama) a la izquierda, codigo a la derecha
- En cada video, empezar con el concepto teorico (2-3 min) y luego ir al codigo
- Mostrar los logs en tiempo real cuando se ejecutan los tests
- Usar diagramas de secuencia para flujos multi-modulo
- Cada video debe poder verse independiente (breve recap al inicio)
- Grabar la terminal con fuente grande (14-16pt) para buena legibilidad
