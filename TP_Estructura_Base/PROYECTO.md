# Arquitectura del TP - Simulador de Sistema Operativo

## Vision General

El proyecto simula un Sistema Operativo con 5 modulos independientes comunicados por sockets TCP usando un protocolo binario basado en opcodes. Cada modulo corre como proceso separado. Usa `so-commons-library` de UTN.

```
  +----------+       +--------+       +----------+
  | CONSOLA  |------>| KERNEL |<----->|   CPU    |
  +----------+  TCP  +--------+  TCP  +----------+
                      |      |              |
                 TCP  |      | TCP     TCP  |
                      v      v              v
                  +-----+  +----+     +----------+
                  |  IO  |  | IO |    | MEMORIA  |
                  +------+  +----+    +----------+
```

**Flujo de vida de un proceso:**
1. **Consola** envia comando RUN al **Kernel**
2. **Kernel** pide a **Memoria** crear el proceso, crea PCB, encola en NEW
3. Planificador largo plazo mueve de NEW a READY
4. Planificador corto plazo selecciona proceso de READY, envia contexto a **CPU**
5. **CPU** ejecuta ciclo fetch-decode-execute
   - Acceso a memoria: pide traduccion/lectura/escritura a **Memoria** (via MMU+TLB)
   - Operacion IO: notifica a **Kernel**, que despacha a la interfaz **IO** correspondiente
6. **CPU** devuelve contexto al **Kernel** con motivo (fin quantum, IO, exit, segfault)
7. **Kernel** maneja la interrupcion, actualiza estado del proceso
8. Se repite hasta que el proceso termina

**Rangos de OpCodes** (`utils/src/protocolo/op_code.h`):
- 0-100: Genericos (handshake, OK, error)
- 200-299: Kernel <-> CPU
- 300-399: Kernel <-> Memoria
- 400-499: Operaciones IO

---

## Modulo: KERNEL

Nucleo del sistema. Administra procesos (PCBs), planificacion, recursos (WAIT/SIGNAL) y coordina todos los demas modulos.

### Archivos

| Archivo | Descripcion |
|---------|-------------|
| `src/main.c` | Entry point. Llama a `kernel_init()` y maneja senales |
| `src/mod_kernel.c/.h` | Contexto global (`KERNEL_CTX`): loggers, sockets, config. Funciones `kernel_init()` y `kernel_shutdown()`. Inicializa conexiones, servidores, planificacion |

#### config/
| Archivo | Descripcion |
|---------|-------------|
| `config/kernel_config.c/.h` | Carga `kernel.config`: IPs, puertos, algoritmo, quantum, grado multiprogramacion, recursos |

#### server/
| Archivo | Descripcion |
|---------|-------------|
| `server/server.c/.h` | Lanza threads para el server de IO y el server de Consola remota |

#### loggers/
| Archivo | Descripcion |
|---------|-------------|
| `loggers/logger.c/.h` | Logs obligatorios del TP: creacion/fin de proceso, cambios de estado, cola ready, page faults. **NO TOCAR** |

#### pcb/
| Archivo | Descripcion |
|---------|-------------|
| `pcb/pcb.c/.h` | Estructura `t_pcb` (PID, estado, PC, registros, quantum, prioridad, path, tam_proceso, `recursos_adquiridos`). Funciones `pcb_crear()`, `pcb_destruir()`, `lista_pids()` |

#### planificacion/
| Archivo | Descripcion |
|---------|-------------|
| `planificacion/planificacion.c/.h` | Modulo principal: colas (NEW, READY, EXEC, BLOCKED, EXIT), mutex, semaforos. Funciones `planificacion_init/start/pause/destroy`, `set_algoritmo()`, `listar_procesos_por_estado()`, `planificacion_finalizar_proceso()` |
| `planificacion/algoritmo.c/.h` | Implementaciones: FIFO (pop primero), RR (FIFO + quantum), VRR (quantum variable), HRRN (ratio respuesta), PRIORIDAD (menor valor = mayor prioridad) |
| `planificacion/corto_plazo.c/.h` | Hilo planificador corto plazo: espera `sem_hay_ready`, selecciona proceso con `proximoAEjecutar()`, lo envia a CPU, espera respuesta |
| `planificacion/largo_plazo.c/.h` | Hilo planificador largo plazo: espera `sem_hay_new` y `sem_mp`, pide a Memoria inicializar proceso, mueve de NEW a READY |

#### conexiones/
| Archivo | Descripcion |
|---------|-------------|
| `conexiones/cpu.c/.h` | Conexion a CPU (dispatch + interrupt). `enviar_contexto_a_cpu()`, `enviar_interrupcion_a_cpu()`, `atender_dispatch_cpu()` (loop que procesa respuestas de CPU), hilo escuchando interrupt |
| `conexiones/cpu_handlers.c/.h` | Handlers para respuestas de CPU: IO_STDIN_READ, IO_STDOUT_WRITE, SEGFAULT, etc. Extraen datos del paquete y llaman a los adaptadores |
| `conexiones/memoria.c/.h` | Conexion a Memoria. `solicitar_init_proceso_memoria()`, `solicitar_fin_proceso_memoria()` |
| `conexiones/io.c/.h` | Server para interfaces IO. Registra interfaces por nombre, `obtener_socket_interfaz()`, maneja multiples conexiones simultaneas |
| `conexiones/consola_handler.c/.h` | Recibe comandos TCP de la consola remota: RUN, KILL, PS, ALGORITMO, START, PAUSE, DESALOJAR, HELP, EXIT |

#### peticiones/
| Archivo | Descripcion |
|---------|-------------|
| `peticiones/proceso.c/.h` | `ejecutar_proceso()`: valida path, crea PCB, encola en NEW. `matar_proceso()`: solicita finalizacion |
| `peticiones/dispatch.c/.h` | `enviar_proceso_a_cpu()`: convierte PCB a contexto y lo envia. `enviar_interrupt_cpu()`: envia interrupcion |
| `peticiones/interrupciones.c/.h` | Dispatcher principal de interrupciones. Recibe motivo de desalojo de CPU y delega al handler correspondiente |
| `peticiones/interrupciones_handlers.c` | Handlers especificos: `manejar_fin_quantum()`, `manejar_fin_proceso()`, `manejar_bloqueo_io()`, `manejar_wait_recurso()`, `manejar_signal_recurso()` |
| `peticiones/recursos.c/.h` | Gestion de recursos con WAIT/SIGNAL. Semaforos por recurso, colas de bloqueados por recurso, `recursos_init/destroy()`, `recursos_liberar_proceso()` |
| `peticiones/ruta_procesos.c/.h` | Validacion de paths de procesos. `construir_nombre_proceso()`, `validar_existe_proceso_kernel()` |

#### deteccion_deadlock/
| Archivo | Descripcion |
|---------|-------------|
| `deteccion_deadlock/grafo_espera.c/.h` | Construye grafo de espera dirigido (proceso_bloqueado → poseedor_recurso) y detecta ciclos con DFS. Se ejecuta al bloquear un proceso por recurso |
| `deteccion_deadlock/banquero.c/.h` | Algoritmo del Banquero: verifica estado seguro simulando liberacion secuencial. Usa `recursos_adquiridos` de cada PCB y colas de bloqueados por recurso |

#### adaptadores/
| Archivo | Descripcion |
|---------|-------------|
| `adaptadores/pcb_cpu_adapter.c/.h` | Convierte PCB <-> contexto CPU (registros, PC) |
| `adaptadores/kernel_memoria_adapter.c/.h` | Adapta requests/responses entre Kernel y Memoria (init, fin, resize) |
| `adaptadores/kernel_io_adapter.c/.h` | Adapta operaciones IO: `kernel_sleep()`, `kernel_stdin_read()`, `kernel_stdout_write()` |
| `adaptadores/kernel_fs_adapter.c` | Operaciones filesystem: CREATE, DELETE, READ, WRITE, TRUNCATE hacia interfaces DialFS |

---

## Modulo: CPU

Ejecuta instrucciones de los procesos. Implementa el ciclo fetch-decode-execute, MMU con TLB, y registros.

### Archivos

| Archivo | Descripcion |
|---------|-------------|
| `src/main.c` | Entry point. Llama a `cpu_init()` |
| `src/cpu.c/.h` | Contexto global (`CPU_CTX`): loggers, sockets, config, registros. Funciones `cpu_init()`, `cpu_shutdown()` |

#### config/
| Archivo | Descripcion |
|---------|-------------|
| `config/cpu_config.c/.h` | Carga `cpu.config`: IP/puerto memoria, IP/puertos kernel (dispatch/interrupt), TLB (cantidad entradas, algoritmo) |

#### server/
| Archivo | Descripcion |
|---------|-------------|
| `server/cpu_server.c/.h` | Lanza hilos para server Dispatch (recibe procesos del Kernel) y server Interrupt (recibe interrupciones) |

#### loggers/
| Archivo | Descripcion |
|---------|-------------|
| `loggers/logger.h` | Declaraciones de logging para CPU |

#### ciclo_instruccion/
| Archivo | Descripcion |
|---------|-------------|
| `ciclo_instruccion/ciclo.c/.h` | Ciclo principal: `ejecutar_ciclo()` -> fetch (pide instruccion a Memoria) -> decode -> execute. Repite hasta interrupcion o instruccion bloqueante |
| `ciclo_instruccion/decode.c/.h` | Parsea string de instruccion a `instruction_t` con opcode y parametros |

#### instrucciones/
| Archivo | Descripcion |
|---------|-------------|
| `instrucciones/instrucciones.h` | Enum de instrucciones: SET, SUM, SUB, JNZ, MOV_IN, MOV_OUT, RESIZE, IO_GEN_SLEEP, IO_STDIN_READ, IO_STDOUT_WRITE, IO_FS_*, WAIT, SIGNAL, EXIT, CALL, RET |
| `instrucciones/operaciones.c/.h` | Ejecuta operaciones aritmeticas/logicas: SET (asignar registro), SUM, SUB, JNZ (salto condicional), CALL, RET |
| `instrucciones/operaciones_mem.c` | Ejecuta operaciones de memoria: MOV_IN (leer de memoria a registro), MOV_OUT (escribir registro a memoria), RESIZE (pedir cambio de tamanio a Memoria) |
| `instrucciones/operaciones_internal.h` | Header interno compartido entre operaciones.c y operaciones_mem.c |

#### registros/
| Archivo | Descripcion |
|---------|-------------|
| `registros/registros.c/.h` | Lectura/escritura generica de registros por nombre. Soporta 8-bit (AX, BX, CX, DX) y 32-bit (EAX, EBX, ECX, EDX, SI, DI) |

#### mmu/
| Archivo | Descripcion |
|---------|-------------|
| `mmu/mmu.c/.h` | Memory Management Unit. `mmu_traducir()`: direccion logica -> fisica. Consulta TLB primero, si miss pide a Memoria. Calcula pagina y offset |

#### tlb/
| Archivo | Descripcion |
|---------|-------------|
| `tlb/tlb.c/.h` | Translation Lookaside Buffer. Cache de traducciones pagina->frame. Algoritmos de reemplazo: FIFO, LRU. `tlb_buscar()`, `tlb_agregar()`, `tlb_limpiar_proceso()` |

#### interrupciones/
| Archivo | Descripcion |
|---------|-------------|
| `interrupciones/interrupciones.c/.h` | Manejo de interrupciones por flag. `check_interrupt()` verifica si hay interrupcion pendiente entre instrucciones |

#### conexiones/
| Archivo | Descripcion |
|---------|-------------|
| `conexiones/cpu_memoria.c/.h` | Establece conexion TCP a Memoria con handshake |

#### adaptadores/
| Archivo | Descripcion |
|---------|-------------|
| `adaptadores/contexto_cpu_adapter.c/.h` | Serializa/deserializa contexto CPU (PID, PC, registros) para envio/recepcion por socket |
| `adaptadores/cpu_dispatch_handler.c/.h` | Handler del canal dispatch: recibe contexto del Kernel, ejecuta ciclo de instrucciones, devuelve resultado con motivo de desalojo |
| `adaptadores/cpu_memoria_adapter.c/.h` | Adapta requests/responses entre CPU y Memoria: fetch instruccion, traducir pagina, leer/escribir memoria |

---

## Modulo: MEMORIA

Administra la memoria RAM, tabla de paginas, traduccion de direcciones, reemplazo de paginas (Clock) y swap a disco.

### Archivos

| Archivo | Descripcion |
|---------|-------------|
| `src/main.c` | Entry point |
| `src/mod_memoria.c/.h` | Contexto global, inicializacion, server loop. Recibe requests de CPU y Kernel, despacha a handlers |

#### configs/
| Archivo | Descripcion |
|---------|-------------|
| `configs/memoria_config.c/.h` | Carga `memoria.config`: tamanio memoria, tamanio pagina, path swap, retardo, esquema |

#### server/
| Archivo | Descripcion |
|---------|-------------|
| `server/server_mem.h` | Interfaz del server de Memoria |

#### gestion/
| Archivo | Descripcion |
|---------|-------------|
| `gestion/memoria_core.c/.h` | Operaciones core: crear/destruir proceso, cargar instrucciones desde archivos, fetch instruccion por PC |
| `gestion/memoria_ram.c/.h` | Gestion de RAM: asignar/liberar frames, leer/escribir bytes en posiciones fisicas |
| `gestion/esquema_memoria.c/.h` | Seleccion de esquema: paginacion o segmentacion, despacha a la implementacion correcta |
| `gestion/paginas.c/.h` | Paginacion: tabla de paginas por proceso, crear/destruir/resize, traducir direccion logica a frame |
| `gestion/paginas_internal.h` | Estructura interna de entrada de tabla de paginas: frame, presente, dirty, use (para Clock) |
| `gestion/paginas_io.c` | Operaciones de paginacion para IO (fetch de instrucciones desde memoria paginada) |
| `gestion/segmentacion.c/.h` | Esquema de segmentacion: base + limite por proceso, read/write/fetch |
| `gestion/reemplazo.c/.h` | Algoritmo de reemplazo de paginas: Clock. Selecciona victima, maneja bit de uso |
| `gestion/frames.c/.h` | Pool de frames: bitmap de frames libres/ocupados, asignar/liberar frame |

#### swap/
| Archivo | Descripcion |
|---------|-------------|
| `swap/swap.c/.h` | Swap a disco: escribir pagina a archivo swap, leer pagina de swap, eliminar swap de proceso |

---

## Modulo: ENTRADA/SALIDA (IO)

Maneja interfaces de IO: generica (sleep), stdin (lectura de teclado), stdout (escritura a pantalla), y dialfs (filesystem).

### Archivos

| Archivo | Descripcion |
|---------|-------------|
| `src/main.c` | Entry point. Recibe nombre de interfaz y path de config como argumentos |

#### core/
| Archivo | Descripcion |
|---------|-------------|
| `core/io_main.c/.h` | Contexto IO, inicializacion: conecta a Kernel, registra interfaz, entra en loop de recepcion |

#### configs/
| Archivo | Descripcion |
|---------|-------------|
| `configs/io_config.c/.h` | Carga config: tipo de interfaz, tiempos de espera, paths para DialFS |

#### server/
| Archivo | Descripcion |
|---------|-------------|
| `server/io_receiver.c` | Loop principal de recepcion de requests. Despacha segun opcode al handler correspondiente |

#### adaptadores/
| Archivo | Descripcion |
|---------|-------------|
| `adaptadores/io_adapter.c/.h` | Handlers para cada operacion: `handle_sleep()`, `handle_stdin_read()`, `handle_stdout_write()`, `handle_fs_create/delete/truncate/write/read()` |

#### interfaces/
| Archivo | Descripcion |
|---------|-------------|
| `interfaces/generic.h` | Header base para IO generica (includes comunes) |
| `interfaces/stdin.h` | Header para IO stdin (includes readline) |
| `interfaces/dialfs.c/.h` | Implementacion de DialFS: filesystem sobre archivo. Crea/elimina archivos, trunca, lee/escribe bloques |
| `interfaces/dialfs_bitmap.c` | Operaciones de bitmap: asignar/liberar bloques contiguos en el archivo de datos |
| `interfaces/dialfs_metadata.c` | Gestion de FCBs (File Control Blocks): crear, buscar, eliminar metadata de archivos |
| `interfaces/dialfs_internal.h` | Estructuras internas: FCB (nombre, bloque inicio, tamanio), superbloque, contexto DialFS |

---

## Modulo: CONSOLA

Interfaz de usuario standalone. Se conecta al Kernel por TCP y envia comandos.

### Archivos

| Archivo | Descripcion |
|---------|-------------|
| `src/main.c` | Entry point |
| `src/consola.c/.h` | REPL principal: lee comandos con readline, parsea, ejecuta |

#### config/
| Archivo | Descripcion |
|---------|-------------|
| `config/consola_config.c/.h` | Carga `consola.config`: IP y puerto del Kernel |

#### helpers/
| Archivo | Descripcion |
|---------|-------------|
| `helpers/consola_helpers.c/.h` | Parseo de comandos (RUN, KILL, PS, ALGORITMO, START, PAUSE, DESALOJAR, EXIT, HELP), menu de ayuda |

#### adaptadores/
| Archivo | Descripcion |
|---------|-------------|
| `adaptadores/consola_kernel_adapter.c/.h` | Serializa y envia comandos al Kernel por TCP |

---

## Modulo: UTILS (Libreria compartida)

Libreria estatica (`libutils.a`) que comparten todos los modulos. Contiene protocolo, serializacion, conexiones y estructuras de datos comunes.

### Archivos

#### model/
| Archivo | Descripcion |
|---------|-------------|
| `model/model.h` | Todas las estructuras compartidas: `registros_t`, `t_contexto_cpu`, `t_motivo_desalojo`, structs de memoria (fetch, traducir, read, write, resize + respuestas), structs de IO (sleep, stdin_read, stdout_write, fs_create, fs_write) |

#### protocolo/
| Archivo | Descripcion |
|---------|-------------|
| `protocolo/op_code.h` | Enum maestro de opcodes: genericos (0-100), Kernel-CPU (200-299), Kernel-Memoria (300-399), IO (400-499) |
| `protocolo/mensajes.c/.h` | Funciones de alto nivel: `enviar_contexto()`, `enviar_interrupcion_cpu()`, `enviar_respuesta()`, `handshake_cliente/servidor()` |

#### paquete/
| Archivo | Descripcion |
|---------|-------------|
| `paquete/paquete.c/.h` | Estructura `t_paquete` (opcode + buffer). API: `paquete_create/destroy()`, `enviar_paquete/recibir_paquete()`, `paquete_write/read_*()` para tipos uint8, uint32, int, bool, string, buffer |

#### serializacion/
| Archivo | Descripcion |
|---------|-------------|
| `serializacion/serializacion.c/.h` | Serializar/deserializar todas las estructuras: contexto CPU, registros, memoria (init, fin, fetch, traducir, read, write, resize + respuestas), IO (sleep, fs_create, fs_write, stdin_read, stdout_write) |

#### conexion/
| Archivo | Descripcion |
|---------|-------------|
| `conexion/conexion.c/.h` | Operaciones de socket: `iniciar_servidor()`, `esperar_cliente()`, `crear_conexion()`, `liberar_conexion()`, `server_escuchar()` (loop generico con thread por cliente), handshakes |

#### common/
| Archivo | Descripcion |
|---------|-------------|
| `common/shared.c/.h` | Utilidades generales: verificar existencia de archivos, leer instrucciones de archivos de texto, manipulacion de arrays de strings |

---

## Sistema de Build

Todos los modulos usan el mismo patron de Makefile:
- **Kernel, CPU, Memoria, IO, Consola**: compilan a ejecutable en `bin/`
- **Utils**: compila a libreria estatica `bin/libutils.a`
- Dependencias automaticas via `STATIC_LIBPATHS=../utils`
- Flags: `-g -Wall -DDEBUG` (debug) o `-O3 -Wall -DNDEBUG` (release)
- Libs: `utils commons pthread readline m`
- Los `.c` se descubren automaticamente con wildcards (no hace falta listar archivos)
