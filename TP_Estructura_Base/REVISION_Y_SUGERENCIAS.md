# Revision Completa del TP Base - Sistema Operativo

## Estado Actual del Proyecto

El TP implementa un simulador de SO con 5 modulos (Kernel, CPU, Memoria, Entrada/Salida, Consola) comunicados por sockets TCP usando un protocolo basado en op_codes. Usa la so-commons-library de UTN.

**Lo que funciona:**
- Ciclo de vida completo de procesos: NEW -> READY -> EXEC -> EXIT
- Planificacion con FIFO, RR (con quantum y desalojo), VRR, HRRN
- Grado de multiprogramacion con semaforo `sem_mp`
- Fetch-Decode-Execute en CPU con comunicacion a Memoria
- Paginacion con tabla de paginas por proceso
- TLB con FIFO y LRU
- Reemplazo de paginas (Clock)
- Swap a disco
- IO Generica (SLEEP)
- Consola remota con HELP, PS, RUN, KILL, PAUSE, START, etc.
- Recursos con WAIT/SIGNAL

---

## 1. HALLAZGOS POR MODULO

### CPU

**Bien:**
- Ciclo fetch-decode-execute bien separado en `ciclo.c`, `decode.c`, `operaciones.c`
- Registros aislados en `registros.c` con lectura/escritura generica
- MMU y TLB correctamente separados
- Logs claros con nombre de registro y resultado de operacion

**A corregir:**

| Archivo | Linea | Problema |
|---------|-------|----------|
| `operaciones.c` | 30 | `ultima_instruccion = *inst` copia el struct incluyendo `parametros` (char*). Si `inst` se libera, `parametros` queda dangling. Para WAIT/SIGNAL que usa `parametros` en el adapter, funciona solo porque se usa inmediatamente, pero es fragil |
| `cpu.h` | - | Define `CPU_CTX` como macro global. Funciona pero acopla todo al nombre de la variable global |
| `ciclo.c` | 27 | Chequea `interrupcion_pendiente()` ANTES del fetch. Si el quantum vence durante execute, la interrupcion se detecta recien en la siguiente iteracion, despues de ejecutar una instruccion extra |
| `decode.c` | - | `parse_registro` usa cadena de if/else en vez de diccionario. Es claro pero ineficiente para muchos registros |
| `contexto_cpu_adapter.c` | 56 | `enviar_contexto(socket_dispatch, ctx, OP_PROCESO_EXEC)` envia siempre `OP_PROCESO_EXEC` como opcode de respuesta, pero el kernel determina el motivo por el opcode que recibe. Esto funciona pero el opcode enviado deberia ser el `rs_code` ya calculado |

**Comentarios sueltos a limpiar:**
- `ciclo.c:52` - "// MMU translation removed as Memory handles PC -> Instruction logic"
- `dispatch.h/dispatch.c` - "// Deprecated?" en `recibir_contexto_actualizado`

---

### Kernel

**Bien:**
- Separacion clara: largo_plazo, corto_plazo, algoritmo, interrupciones, recursos
- PCB con todos los campos necesarios (pid, pc, registros, quantum, estado, etc.)
- Manejo de consola remota bien implementado
- PAUSE/START con condition variable funciona correctamente

**A corregir:**

| Archivo | Linea | Problema |
|---------|-------|----------|
| `interrupciones.c` | 78-111 | `manejar_interrupcion` usa strcmp para motivos ("QUANTUM", "IO", "EXIT"). Deberia usar un enum |
| `interrupciones.c` | 106 | `cola_exit` usa `mutex_exec` en vez de un mutex propio. Comentario dice "Reusamos mutex exec para cola exit por simplicidad" |
| `planificacion.c` | 239 | `cola_exit` tambien usa `mutex_exec`. Inconsistente y propenso a deadlocks si crece |
| `corto_plazo.c` | 80-81 | Reutiliza `tiempo_ready` como "marcador de inicio exec" para VRR. Deberia ser un campo separado en el PCB (`tiempo_inicio_exec`) |
| `cpu.c` | - | El hilo de dispatch lee del socket, pero si CPU se desconecta no hay reconexion ni recovery |
| `io.c` | - | Variables estaticas `_buscar_nombre`, `_buscar_socket` para closures de so-commons. Funciona pero no es thread-safe si dos hilos buscan interfaces al mismo tiempo |
| `consola_handler.c` | 94-96 | PS toma cada cola con lock individual. Snapshot no es atomico (un proceso puede aparecer en dos colas o en ninguna) |
| `proceso.c` | - | `ejecutar_proceso` y `matar_proceso` deberian validar PID > 0 |

**Comentarios sueltos:**
- `planificacion.c:20-21` - `// extern t_log* logger;` y `// extern t_kernel_config KCONF;` comentados
- `largo_plazo.c:40-51` - Bloque de comentarios sobre manejo de error con "UTN Hack", "Better call..." - limpiar
- `planificacion.c:218-241` - Comentarios extensos sobre manejo de BLOCKED en kill que son notas de implementacion

---

### Memoria

**Bien:**
- Paginacion completa con tabla por proceso (dictionary)
- Frame management con bitmap (bitarray)
- Swap implementado
- Mutex en paginas y frames

**A corregir (CRITICO):**

| Archivo | Linea | Problema |
|---------|-------|----------|
| `reemplazo.c` | 45-48 | **Algoritmo Clock roto**: usa `dictionary_iterator` que no mantiene un puntero circular real. Itera desde el inicio cada vez, violando la semantica de Clock. Es mas bien un "segunda oportunidad lineal" |
| `paginas.c` | 249-256 | **Race condition**: `paginacion_obtener_entrada` libera el mutex, luego `paginacion_escribir` lo vuelve a tomar para modificar flags. Entre ambos, otro hilo podria desalojar esa pagina |
| `memoria_core.c` | 128-141 | **Doble almacenamiento**: instrucciones estan tanto en `proc->instrucciones[]` (array) como en paginas. El fetch usa paginacion con fallback al array. Deberia usar SOLO paginacion o SOLO el array |
| `memoria.config` | 6 | `ALGORITMO_REEMPLAZO=FIFO` pero el codigo solo implementa Clock. Config enganiosa |
| `segmentacion.c` | todo | Modulo stub con datos mock (`seg->base = pid * 1000`). Nunca se usa. Eliminar o marcar claramente |
| `paginas.h` | 31 | `dump_paginas` declarada pero nunca implementada |
| `memoria_ram.h` | 15-16 | `get_memoria_espacio()` marcada como "SOLO PARA DEBUG - Dangerous" en header publico |

**Conceptual:**
- `memoria_core.c:152` - Cuando el PC es invalido, retorna `strdup("EXIT")`. Esto es semanticamente incorrecto: un PC invalido deberia ser un error, no una instruccion valida

---

### Entrada/Salida

**Bien:**
- Soporte para 4 tipos de interfaz (GENERICA, STDIN, STDOUT, DIALFS)
- Config flexible con campos opcionales (IP_MEMORIA solo si necesita)
- Estructura modular con adapter, receiver, config separados

**A corregir:**

| Archivo | Linea | Problema |
|---------|-------|----------|
| `dialfs.h` | - | Estructuras y funciones declaradas pero implementacion parcial |
| `io_main.c` | - | El switch de tipo de interfaz es largo. Podria usar un dispatch table |
| `generica.c` | - | `usleep(unidades * 1000)` - el TP tipicamente pide que las unidades de trabajo se multipliquen por un valor de config, no hardcodeado a 1ms |

---

### Utils (Biblioteca compartida)

**Bien:**
- Protocolo de paquetes bien encapsulado (crear, escribir, leer, destruir)
- Op_codes centralizados
- Funciones de serializacion separadas

**A corregir:**

| Archivo | Linea | Problema |
|---------|-------|----------|
| `op_code.h` | - | Mezcla op_codes de todos los modulos en un solo enum. Funciona pero crece indefinidamente. Considerar agrupar por rangos (100-199 kernel, 200-299 cpu, 300-399 memoria) - ya se hace parcialmente |
| `shared.c` | - | `leer_instrucciones` abre archivo y lee linea por linea. No valida lineas vacias al final del archivo |

---

### Consola

**Bien:**
- Usa readline con historial
- Comandos parseados con dictionary
- HELP funcional
- Manejo limpio de shutdown

**Sin problemas criticos.**

---

## 2. ERRORES CONCEPTUALES PRINCIPALES

1. **Clock Algorithm (reemplazo.c)**: No es un clock real. `dictionary_iterator` no mantiene la posicion de la "manecilla". Cada page fault empieza a buscar desde el primer proceso del diccionario, no desde donde quedo la ultima vez.

2. **Doble almacenamiento de instrucciones (memoria_core.c)**: Las instrucciones se guardan en un array Y en paginas. El fetch intenta leer de paginas y si falla usa el array. En un SO real, solo estarian en memoria virtual (paginas/swap).

3. **EXIT como error (memoria_core.c:152)**: Retornar "EXIT" cuando el PC es invalido hace que el proceso termine "limpiamente" cuando en realidad deberia ser un SEGFAULT o error fatal.

4. **Motivos como strings (interrupciones.c)**: Usar `strcmp(motivo, "QUANTUM")` para determinar acciones es fragil. Un typo rompe todo silenciosamente.

5. **PS no atomico (consola_handler.c)**: Cada cola se lee con su propio lock. Un proceso puede aparecer en dos estados o en ninguno en el snapshot.

---

## 3. SUGERENCIAS PARA FUTURO TP / BASE

### Corto plazo (antes de usar como base)
- [ ] Limpiar comentarios de implementacion (TODO, HACK, "UNIFICACION", codigo comentado)
- [ ] Eliminar `segmentacion.c` o moverlo a una carpeta `unused/`
- [ ] Corregir el config de memoria (`ALGORITMO_REEMPLAZO=CLOCK`)
- [ ] Agregar mutex propio para `cola_exit` en vez de reusar `mutex_exec`
- [ ] Usar enum en vez de strings para motivos de interrupcion

### Mediano plazo (mejoras de arquitectura)
- [ ] Eliminar el fallback de instrucciones en `memoria_fetch_instruccion` (usar solo paginacion)
- [ ] Implementar Clock real con puntero circular persistente
- [ ] Proteger `socket_memoria` en CPU con mutex (similar al fix del kernel)
- [ ] Agregar campo `tiempo_inicio_exec` en PCB en vez de reusar `tiempo_ready`
- [ ] Hacer PS atomico (tomar todos los locks antes de leer)

### Ideas para futuros TPs

1. **Filesystem**: DIALFS ya tiene la estructura base. Extender con operaciones reales de create/delete/truncate/read/write sobre un archivo de bloques
2. **Memoria virtual completa**: Remover el array de instrucciones y que TODO pase por paginacion+swap. Implementar demand paging real
3. **Planificacion avanzada**: Agregar SJF, SRT, Prioridades con envejecimiento. La estructura `set_algoritmo` ya soporta esto facilmente
4. **Deadlock detection**: Con los recursos (WAIT/SIGNAL) ya implementados, agregar deteccion de deadlock con grafo de recursos
5. **Multihilo (threads)**: Agregar instrucciones THREAD_CREATE, THREAD_JOIN, THREAD_EXIT. Compartir tabla de paginas entre hilos del mismo proceso
6. **Signals e IPC**: PIPE, semaforos con nombre, shared memory entre procesos
7. **Consola interactiva mejorada**: Agregar comandos como `DUMP <pid>` (ver registros/paginas), `MEM` (ver uso de frames/swap), `TOP` (procesos ordenados por CPU time)
8. **Metricas**: Tiempo de respuesta, turnaround, waiting time por proceso. Exportar a CSV para comparar algoritmos
9. **Tests automatizados**: Scripts que lanzan secuencias de procesos y verifican estados esperados. Util para regression testing entre TPs
10. **Video explicativo**: El proyecto ya tiene buen material para explicar:
    - Arquitectura general (5 modulos + sockets)
    - Ciclo de vida de un proceso (consola -> kernel -> memoria -> cpu -> kernel)
    - Paginacion y page faults
    - Planificacion RR con quantum y desalojo
    - Grado de multiprogramacion

### Para video explicativo recomiendo cubrir:
1. **Intro**: Diagrama de modulos y puertos, como se conectan
2. **Demo live**: Levantar modulos, RUN proceso simple, mostrar logs en cada terminal
3. **Profundizar**: RUN infinito, mostrar quantum/desalojo, PS, grado multiprogramacion
4. **Codigo**: Mostrar el flujo de un paquete: consola envia RUN -> kernel crea PCB -> memoria carga instrucciones -> CPU ejecuta -> kernel recibe resultado
5. **Conceptos**: Relacionar cada parte del codigo con la teoria (planificador, PCB, tabla de paginas, TLB, etc.)
