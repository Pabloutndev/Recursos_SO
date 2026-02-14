# Casos de Uso End-to-End

Documento que describe cada flujo completo del sistema, paso a paso, indicando
**quién envía**, **qué op_code**, **a quién**, y **qué datos** se transmiten.

---

## 1. CICLO DE VIDA DE UN PROCESO (Happy Path)

### 1.1 Creación de Proceso

```
CONSOLA                    KERNEL                     MEMORIA
   │                         │                           │
   │  RUN <nombre>           │                           │
   │────────────────────────>│                           │
   │                         │  pcb_crear(path)          │
   │                         │  estado = NEW             │
   │                         │  list_add(cola_new, pcb)  │
   │                         │  sem_post(sem_hay_new)    │
   │                         │                           │
   │                 [Largo Plazo despierta]              │
   │                         │  sem_wait(sem_hay_new)    │
   │                         │  sem_wait(sem_mp)         │
   │                         │                           │
   │                         │  OP_MEM_INIT_PROCESO      │
   │                         │  {pid, path}              │
   │                         │──────────────────────────>│
   │                         │                           │  memoria_crear_proceso()
   │                         │                           │  paginacion_crear_proceso()
   │                         │                           │  cargar instrucciones
   │                         │        OP_OK              │
   │                         │<──────────────────────────│
   │                         │                           │
   │                         │  estado = READY           │
   │                         │  list_add(cola_ready)     │
   │                         │  sem_post(sem_hay_ready)  │
```

### 1.2 Ejecución (Dispatch a CPU)

```
KERNEL                      CPU                        MEMORIA
   │                         │                           │
   │  [Corto Plazo]          │                           │
   │  sem_wait(sem_hay_ready)│                           │
   │  algoritmo selecciona   │                           │
   │  estado = EXEC          │                           │
   │                         │                           │
   │  OP_PROCESO_EXEC        │                           │
   │  {pid, pc, registros}   │                           │
   │────────────────────────>│                           │
   │                         │  mmu_set_contexto()       │
   │                         │  while(true):             │
   │                         │                           │
   │                         │    OP_MEM_FETCH_INSTRUC.  │
   │                         │    {pid, pc}              │
   │                         │──────────────────────────>│
   │                         │                           │  buscar instruccion[pc]
   │                         │    OP_MEM_RESP_INSTRUC.   │
   │                         │    {instruccion_string}   │
   │                         │<──────────────────────────│
   │                         │                           │
   │                         │    decode(instruccion)    │
   │                         │    execute(instruccion)   │
   │                         │    pc++                   │
   │                         │                           │
```

### 1.3 Fin de Proceso (EXIT)

```
CPU                         KERNEL                     MEMORIA
   │                         │                           │
   │  decode: "EXIT"         │                           │
   │  motivo = MOTIVO_EXIT   │                           │
   │                         │                           │
   │  OP_CPU_FIN_PROCESO     │                           │
   │  {pid, pc, registros}   │                           │
   │────────────────────────>│                           │
   │                         │  manejar_fin_proceso()    │
   │                         │  estado = EXIT            │
   │                         │  sem_post(sem_mp)         │
   │                         │                           │
   │                         │  OP_MEM_FIN_PROCESO       │
   │                         │  {pid}                    │
   │                         │──────────────────────────>│
   │                         │                           │  paginacion_destruir()
   │                         │                           │  swap_borrar()
   │                         │                           │  memoria_destruir()
   │                         │  pcb_destruir()           │
```

---

## 2. FIN DE QUANTUM (Round Robin)

```
KERNEL                      CPU
   │                         │
   │  [timer_quantum thread] │
   │  usleep(quantum_ms)     │
   │                         │
   │  OP_INTERRUPCION_CPU    │
   │────────────────────────>│  (canal INTERRUPT)
   │                         │
   │                         │  flag_interrupcion = true
   │                         │  ciclo detecta flag
   │                         │  break del while
   │                         │
   │  OP_FIN_DE_QUANTUM      │
   │  {pid, pc, registros}   │
   │<────────────────────────│  (canal DISPATCH)
   │                         │
   │  manejar_fin_quantum()  │
   │  actualizar PCB con ctx │
   │  estado = READY         │
   │  list_add(cola_ready)   │
   │  sem_post(sem_hay_ready)│
```

---

## 3. OPERACIÓN I/O GENERICA (IO_GEN_SLEEP)

```
CPU                    KERNEL                   I/O (GENERICA)
   │                     │                           │
   │ decode:             │                           │
   │ "IO_GEN_SLEEP       │                           │
   │  INTERFAZ 5"        │                           │
   │ motivo=MOTIVO_IO    │                           │
   │ pc++                │                           │
   │                     │                           │
   │ OP_IO_SLEEP         │                           │
   │ {pid,pc,regs}       │                           │
   │────────────────────>│                           │
   │                     │  actualizar PCB           │
   │                     │  estado = BLOCKED         │
   │                     │  buscar interfaz socket   │
   │                     │                           │
   │                     │  OP_IO_SLEEP              │
   │                     │  {pid, tiempo_ms}         │
   │                     │──────────────────────────>│
   │                     │                           │  usleep(tiempo*unidad)
   │                     │                           │
   │                     │  OP_IO_FIN_OPERACION      │
   │                     │  {pid}                    │
   │                     │<──────────────────────────│
   │                     │                           │
   │                     │  manejar_fin_io()         │
   │                     │  estado = READY           │
   │                     │  sem_post(sem_hay_ready)  │
```

---

## 4. OPERACIÓN STDIN (IO_STDIN_READ)

```
CPU               KERNEL              I/O (STDIN)           MEMORIA
   │                │                     │                    │
   │ "IO_STDIN_READ │                     │                    │
   │  TECLADO       │                     │                    │
   │  DirReg        │                     │                    │
   │  SizeReg"      │                     │                    │
   │ motivo=IO      │                     │                    │
   │                │                     │                    │
   │ OP_IO_STDIN_RD │                     │                    │
   │ {pid,pc,regs}  │                     │                    │
   │───────────────>│                     │                    │
   │                │  estado=BLOCKED     │                    │
   │                │                     │                    │
   │                │  OP_IO_STDIN_READ   │                    │
   │                │  {pid,dir_log,size} │                    │
   │                │────────────────────>│                    │
   │                │                     │  readline("> ")    │
   │                │                     │  (usuario escribe) │
   │                │                     │                    │
   │                │                     │  OP_MEM_ESCRIBIR   │
   │                │                     │  {pid,dir,data}    │
   │                │                     │───────────────────>│
   │                │                     │                    │  escribir en RAM
   │                │                     │  OP_OK             │
   │                │                     │<───────────────────│
   │                │                     │                    │
   │                │  OP_IO_FIN_OPERAC.  │                    │
   │                │  {pid}              │                    │
   │                │<────────────────────│                    │
   │                │  estado=READY       │                    │
```

---

## 5. OPERACIÓN STDOUT (IO_STDOUT_WRITE)

```
CPU               KERNEL              I/O (STDOUT)          MEMORIA
   │                │                     │                    │
   │ "IO_STDOUT_WR  │                     │                    │
   │  MONITOR       │                     │                    │
   │  DirReg        │                     │                    │
   │  SizeReg"      │                     │                    │
   │ motivo=IO      │                     │                    │
   │                │                     │                    │
   │ OP_IO_STDOUT   │                     │                    │
   │ {pid,pc,regs}  │                     │                    │
   │───────────────>│                     │                    │
   │                │  estado=BLOCKED     │                    │
   │                │                     │                    │
   │                │  OP_IO_STDOUT_WRITE │                    │
   │                │  {pid,dir_log,size} │                    │
   │                │────────────────────>│                    │
   │                │                     │                    │
   │                │                     │  OP_MEM_LEER       │
   │                │                     │  {pid,dir,size}    │
   │                │                     │───────────────────>│
   │                │                     │                    │  leer de RAM
   │                │                     │  OP_MEM_RESP_LECT  │
   │                │                     │  {ok, data, size}  │
   │                │                     │<───────────────────│
   │                │                     │                    │
   │                │                     │  printf(data)      │
   │                │                     │                    │
   │                │  OP_IO_FIN_OPERAC.  │                    │
   │                │  {pid}              │                    │
   │                │<────────────────────│                    │
   │                │  estado=READY       │                    │
```

---

## 6. ACCESO A MEMORIA (MOV_IN / MOV_OUT)

### 6.1 MOV_IN (Leer de memoria a registro)

```
CPU                                     MEMORIA
   │                                       │
   │  dir_logica = REG[r2]                 │
   │  pagina = dir_logica / tam_pag        │
   │  offset = dir_logica % tam_pag        │
   │                                       │
   │  TLB lookup(pid, pagina)              │
   │  HIT? → dir_fisica = marco*tam+off   │
   │  MISS? ↓                              │
   │                                       │
   │  OP_MEM_TRADUCIR_PAGINA               │
   │  {pid, dir_logica}                    │
   │──────────────────────────────────────>│
   │                                       │  page fault? → reemplazo clock
   │                                       │  swap in si necesario
   │  OP_MEM_RESP_TRADUCCION               │
   │  {ok=true, dir_fisica}                │
   │<──────────────────────────────────────│
   │                                       │
   │  TLB update(pid, pagina, marco)       │
   │  dir_fisica = marco*tam + offset      │
   │                                       │
   │  OP_MEM_LEER                          │
   │  {pid, dir_fisica, size}              │
   │──────────────────────────────────────>│
   │                                       │  leer_memoria_fisica()
   │  OP_MEM_RESP_LECTURA                  │
   │  {ok=true, data, size}                │
   │<──────────────────────────────────────│
   │                                       │
   │  REG[r1] = data                       │
   │  pc++                                 │
```

### 6.2 MOV_OUT (Escribir de registro a memoria)

```
CPU                                     MEMORIA
   │                                       │
   │  dir_logica = REG[r1]                 │
   │  valor = REG[r2]                      │
   │  traducir via MMU/TLB (igual que 6.1) │
   │                                       │
   │  OP_MEM_ESCRIBIR                      │
   │  {pid, dir_fisica, size, buffer}      │
   │──────────────────────────────────────>│
   │                                       │  escribir_memoria_fisica()
   │                                       │  marcar dirty bit
   │  OP_OK                                │
   │<──────────────────────────────────────│
   │                                       │
   │  pc++                                 │
```

---

## 7. RECURSOS (WAIT / SIGNAL)

### 7.1 WAIT (Recurso disponible - no bloquea)

```
CPU                         KERNEL
   │                         │
   │ decode: "WAIT REC1"     │
   │ motivo = MOTIVO_IO      │
   │ pc++                    │
   │                         │
   │ OP_WAIT_RECURSO         │
   │ {pid, pc, regs}         │
   │ parametros="REC1"       │
   │────────────────────────>│
   │                         │  recurso_wait(pcb, "REC1")
   │                         │  instancias > 0 → instancias--
   │                         │  return false (no bloquea)
   │                         │
   │                         │  re-dispatch a CPU
   │                         │  estado = EXEC
   │ OP_PROCESO_EXEC         │
   │ {pid, pc, regs}         │
   │<────────────────────────│
```

### 7.2 WAIT (Recurso NO disponible - bloquea)

```
CPU                         KERNEL
   │                         │
   │ OP_WAIT_RECURSO         │
   │ {pid, pc, regs}         │
   │────────────────────────>│
   │                         │  recurso_wait(pcb, "REC1")
   │                         │  instancias == 0
   │                         │  add pcb a cola_bloqueados del recurso
   │                         │  estado = BLOCKED
   │                         │  return true (bloqueado)
   │                         │
   │                         │  [proceso queda bloqueado
   │                         │   hasta que otro haga SIGNAL]
```

### 7.3 SIGNAL (Libera recurso y desbloquea proceso)

```
CPU                         KERNEL
   │                         │
   │ decode: "SIGNAL REC1"   │
   │ motivo = MOTIVO_IO      │
   │ pc++                    │
   │                         │
   │ OP_SIGNAL_RECURSO       │
   │ {pid, pc, regs}         │
   │────────────────────────>│
   │                         │  recurso_signal("REC1")
   │                         │  hay bloqueados? →
   │                         │    desbloqueado = list_remove(cola, 0)
   │                         │    desbloqueado.estado = READY
   │                         │    list_add(cola_ready, desbloqueado)
   │                         │    sem_post(sem_hay_ready)
   │                         │  no hay bloqueados? →
   │                         │    instancias++
   │                         │
   │                         │  proceso actual (el que hizo SIGNAL)
   │                         │  re-dispatch a CPU
```

---

## 8. RESIZE

```
CPU                                     MEMORIA
   │                                       │
   │ decode: "RESIZE 512"                  │
   │                                       │
   │ OP_MEM_AJUSTAR_TAMANIO                │
   │ {pid, nuevo_tamanio=512}              │
   │──────────────────────────────────────>│
   │                                       │  paginacion_resize(pid, 512)
   │                                       │  agregar/quitar páginas
   │  OP_OK (o OP_FAIL si Out of Memory)   │
   │<──────────────────────────────────────│
   │                                       │
   │  OK → pc++, continuar                 │
   │  FAIL → MOTIVO_SEGFAULT              │
```

---

## 9. COPY_STRING

```
CPU                                     MEMORIA
   │                                       │
   │ decode: "COPY_STRING 10"              │
   │ origen = REG[SI]                      │
   │ destino = REG[DI]                     │
   │                                       │
   │ traducir(origen) → df_origen          │
   │ (via MMU/TLB, igual que MOV)          │
   │                                       │
   │ OP_MEM_LEER                           │
   │ {pid, df_origen, 10}                  │
   │──────────────────────────────────────>│
   │ OP_MEM_RESP_LECTURA {data}            │
   │<──────────────────────────────────────│
   │                                       │
   │ traducir(destino) → df_destino        │
   │                                       │
   │ OP_MEM_ESCRIBIR                       │
   │ {pid, df_destino, 10, data}           │
   │──────────────────────────────────────>│
   │ OP_OK                                 │
   │<──────────────────────────────────────│
   │                                       │
   │ pc++                                  │
```

---

## 10. KILL DE PROCESO (Desde consola)

```
CONSOLA                    KERNEL                     CPU              MEMORIA
   │                         │                         │                  │
   │  KILL <pid>             │                         │                  │
   │────────────────────────>│                         │                  │
   │                         │  buscar PCB por pid     │                  │
   │                         │                         │                  │
   │  [Si esta en EXEC]      │  OP_INTERRUPCION_CPU    │                  │
   │                         │────────────────────────>│                  │
   │                         │                         │ flag=true        │
   │                         │  OP_FIN_DE_QUANTUM      │                  │
   │                         │<────────────────────────│                  │
   │                         │                         │                  │
   │  [En cualquier estado]  │                         │                  │
   │                         │  estado = EXIT          │                  │
   │                         │  remover de su cola     │                  │
   │                         │  sem_post(sem_mp)       │                  │
   │                         │                         │                  │
   │                         │  OP_MEM_FIN_PROCESO     │                  │
   │                         │  {pid}                  │                  │
   │                         │────────────────────────────────────────────>│
   │                         │                         │                  │ cleanup
   │                         │  pcb_destruir()         │                  │
   │                         │  recursos_liberar(pid)  │                  │
```

---

## 11. SEGFAULT

```
CPU                         KERNEL
   │                         │
   │ MOV_IN REG, DIR_INVALIDA│
   │ mmu_traducir() → ERROR  │
   │ motivo = MOTIVO_SEGFAULT│
   │                         │
   │ OP_SEGFAULT              │
   │ {pid, pc, regs}         │
   │────────────────────────>│
   │                         │  manejar_segfault()
   │                         │  estado = EXIT
   │                         │  sem_post(sem_mp)
   │                         │  solicitar_fin_proceso_memoria(pid)
   │                         │  pcb_destruir()
   │                         │
   │                         │  LOG: "PID X - SEGFAULT"
```

---

## 12. HANDSHAKE (Conexión inicial entre módulos)

```
CLIENTE                     SERVIDOR
   │                         │
   │  crear_conexion(ip,pto) │
   │  TCP connect            │
   │────────────────────────>│  accept()
   │                         │
   │  OP_HANDSHAKE           │
   │  (paquete vacío)        │
   │────────────────────────>│
   │                         │  handshake_servidor()
   │  OP_OK                  │
   │  (paquete vacío)        │
   │<────────────────────────│
   │                         │
   │  handshake_cliente()    │
   │  valida OP_OK           │
   │  return true            │
```

Para I/O el handshake incluye el nombre:
```
I/O                         KERNEL
   │                         │
   │  OP_HANDSHAKE_IO        │
   │  {nombre_interfaz}      │
   │────────────────────────>│  registrar interfaz + socket
   │                         │
   │  OP_OK                  │
   │<────────────────────────│
```

---

## 13. TABLA DE OP_CODES

| Op Code | Valor | Dirección | Datos |
|---------|-------|-----------|-------|
| OP_OK | 1 | Resp. genérica | (vacío) |
| OP_FAIL | 0 | Resp. genérica | (vacío) |
| OP_HANDSHAKE | 5 | Bidireccional | (vacío) |
| OP_HANDSHAKE_IO | 6 | IO→Kernel | {nombre_interfaz} |
| OP_PROCESO_EXEC | 200 | Kernel→CPU | {pid, pc, registros} |
| OP_INTERRUPCION_CPU | 201 | Kernel→CPU | (vacío) |
| OP_FIN_DE_QUANTUM | 202 | CPU→Kernel | {pid, pc, registros} |
| OP_CPU_FIN_PROCESO | 203 | CPU→Kernel | {pid, pc, registros} |
| OP_DESALOJO | 204 | CPU→Kernel | {pid, pc, registros} |
| OP_SEGFAULT | 205 | CPU→Kernel | {pid, pc, registros} |
| OP_MEM_INIT_PROCESO | 300 | Kernel→Mem | {pid, path} |
| OP_MEM_FIN_PROCESO | 301 | Kernel→Mem | {pid} |
| OP_MEM_TRADUCIR_PAGINA | 302 | CPU→Mem | {pid, dir_logica} |
| OP_MEM_LEER | 303 | CPU/IO→Mem | {pid, dir_logica, size} |
| OP_MEM_ESCRIBIR | 304 | CPU/IO→Mem | {pid, dir_logica, size, buffer} |
| OP_MEM_FETCH_INSTRUCCION | 305 | CPU→Mem | {pid, pc} |
| OP_MEM_AJUSTAR_TAMANIO | 306 | CPU→Mem | {pid, nuevo_tamanio} |
| OP_MEM_RESP_TRADUCCION | 307 | Mem→CPU | {ok, dir_fisica} |
| OP_MEM_RESP_INSTRUCCION | 308 | Mem→CPU | {instruccion_string} |
| OP_MEM_RESP_LECTURA | 309 | Mem→CPU/IO | {ok, size, data} |
| OP_IO_SLEEP | 400 | Kernel→IO | {pid, tiempo} |
| OP_IO_FS_CREATE | 401 | Kernel→IO | {pid, path} |
| OP_IO_FS_DELETE | 402 | Kernel→IO | {pid, path} |
| OP_IO_FS_TRUNCATE | 403 | Kernel→IO | {pid, path, tamanio} |
| OP_IO_FS_WRITE | 404 | Kernel→IO | {pid, path, offset, size} |
| OP_IO_FS_READ | 405 | Kernel→IO | {pid, path, offset, size} |
| OP_IO_FIN_OPERACION | 406 | IO→Kernel | {pid} |
| OP_SIGNAL_RECURSO | 407 | CPU→Kernel | {pid, pc, regs} + nombre en params |
| OP_WAIT_RECURSO | 408 | CPU→Kernel | {pid, pc, regs} + nombre en params |
| OP_IO_STDIN_READ | 410 | Kernel→IO | {pid, dir_logica, size, interfaz} |
| OP_IO_STDOUT_WRITE | 411 | Kernel→IO | {pid, dir_logica, size, interfaz} |

---

## 14. ESTADOS DE PROCESO

```
     ┌─────┐
     │ NEW │ ← RUN desde consola
     └──┬──┘
        │ Largo Plazo (Memoria OK + grado MP)
        ▼
     ┌───────┐
  ┌─>│ READY │<─────────────────────────────────────┐
  │  └──┬────┘                                      │
  │     │ Corto Plazo (FIFO/RR/HRRN selecciona)     │
  │     ▼                                           │
  │  ┌──────┐  fin quantum (RR)                     │
  │  │ EXEC │──────────────────────────────────────>│
  │  └──┬───┘                                       │
  │     │                                           │
  │     ├── EXIT ──────────> ┌──────┐               │
  │     │                    │ EXIT │               │
  │     │                    └──────┘               │
  │     │                                           │
  │     ├── SEGFAULT ──────> ┌──────┐               │
  │     │                    │ EXIT │               │
  │     │                    └──────┘               │
  │     │                                           │
  │     ├── I/O ───────────> ┌─────────┐            │
  │     │                    │ BLOCKED │────────────>│
  │     │                    └─────────┘  I/O FIN   │
  │     │                                           │
  │     └── WAIT(recurso) ─> ┌─────────┐            │
  │                          │ BLOCKED │────────────┘
  │                          └─────────┘  SIGNAL
  │
  └── SIGNAL (proceso que señaliza vuelve a READY)
```

---

## 15. MODELO DE HILOS

### Kernel (6+ hilos)
- **Main**: Consola interactiva (readline)
- **Hilo Largo Plazo**: NEW → READY
- **Hilo Corto Plazo**: READY → EXEC (bloquea en dispatch)
- **Hilo Timer Quantum**: usleep + interrupción (1 por proceso en EXEC, detached)
- **Hilo Server I/O**: accept() para interfaces I/O
- **Hilo por interfaz I/O**: handler de cada interfaz conectada

### CPU (3 hilos)
- **Main**: sleep loop
- **Hilo Dispatch**: recibe OP_PROCESO_EXEC, ejecuta ciclo, devuelve contexto
- **Hilo Interrupt**: recibe OP_INTERRUPCION_CPU, setea flag

### Memoria (1+ hilos)
- **Main**: server_listen_loop()
- **Hilo por conexión**: handler para CPU y Kernel (pthread por accept)

### I/O (1 hilo principal)
- **Main**: io_receiver_loop() escucha a Kernel
