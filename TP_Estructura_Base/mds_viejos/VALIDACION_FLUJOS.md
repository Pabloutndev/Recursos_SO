# Validación de Flujos de Comunicación

## ✅ Estado Actual Después de Refactoring

### 1. KERNEL ↔ CPU

#### Flujo: Kernel Dispatch → CPU

```
KERNEL (Planificador)
├─ Selecciona PCB
└─ Llama: kernel_cpu_adapter_dispatch(pcb)
    ├─ Convierte: pcb → t_contexto_cpu
    ├─ Envía: enviar_contexto(socket_cpu_dispatch, ctx, OP_PROCESO_EXEC)
    │   └─ PROTOCOLO: serializa + envía paquete
    └─ Retorna (no espera respuesta, CPU ejecutará asincronicamente)

    ↓ RED ↓

CPU (Servidor DISPATCH)
├─ Recibe paquete (OP_PROCESO_EXEC)
├─ Deserializa: t_contexto_cpu* ctx = recibir_contexto(paquete)
├─ Llama: ciclo_instruccion_ejecutar(ctx)
│   ├─ Ejecuta instrucciones
│   ├─ Actualiza registros, PC
│   └─ Determina motivo de devolución (Quantum, IO, Exit, etc.)
└─ Envía respuesta a Kernel (contexto actualizado + motivo)

    ↓ RED ↓

KERNEL (Servidor CPU - Dispatch Listener)
├─ Recibe contexto devuelto
├─ Llama: contexto_cpu_a_pcb(ctx, pcb)
├─ Procesa motivo (FIN_QUANTUM, IO, EXIT, etc.)
└─ Continúa ejecución según planificación
```

**Estado: ✅ CORRECTO**
- Adapter prepara contexto
- Protocolo serializa
- Servidor solo recibe y delega a ciclo
- Respuesta se maneja en listener de Kernel

---

#### Flujo: Kernel Interrupt → CPU

```
KERNEL
├─ Razón: FIN_QUANTUM, BLOQUEO_IO, etc.
└─ Llama: kernel_cpu_adapter_interrupt()
    ├─ Envía: enviar_interrupcion_cpu(socket_cpu_interrupt)
    └─ Retorna inmediatamente

CPU (Servidor INTERRUPT)
├─ Recibe señal
├─ Detiene ejecución actual
└─ Devolve contexto a Kernel
```

**Estado: ✅ CORRECTO**
- Interrupt es señal one-way
- CPU se interrumpe y devuelve contexto

---

### 2. KERNEL ↔ MEMORIA

#### Flujo: Kernel → Memoria (INIT_PROCESO)

```
KERNEL (crear_proceso)
├─ Crea PCB
└─ Llama: kernel_memoria_adapter_init_proceso(pcb)
    ├─ Convierte: pcb → t_mem_init_proceso
    ├─ Envía: enviar_init_proceso(socket_memoria, req, OP_MEM_INIT_PROCESO)
    ├─ Espera respuesta (int: 1=OK, 0=FAIL)
    └─ Retorna bool

    ↓ RED ↓

MEMORIA (Servidor)
├─ Recibe paquete (OP_MEM_INIT_PROCESO)
├─ Deserializa: t_mem_init_proceso* req = recibir_init_proceso(paquete)
├─ Delega: memoria_adapter_init_proceso(req, fd)
│   ├─ Llama: paginacion_crear_proceso(pid, size)
│   └─ Envía: enviar_respuesta_ok(fd) o enviar_respuesta_fail(fd)
└─ Espera siguiente request

KERNEL
├─ Recibe respuesta (1 u 0)
├─ Si OK: agrega PCB a NEW queue
└─ Si FAIL: error, retorna false
```

**Estado: ✅ CORRECTO**
- Adapter forma estructura compartida
- Protocolo serializa/deserializa
- Adapter procesa y responde
- Kernel recibe respuesta bloqueante

---

#### Flujo: Kernel → Memoria (FIN_PROCESO)

```
KERNEL
└─ Llama: kernel_memoria_adapter_fin_proceso(pid)
    └─ Envía: enviar_fin_proceso(socket_memoria, req, OP_MEM_FIN_PROCESO)
        └─ ONE-WAY (no espera respuesta)

    ↓ RED ↓

MEMORIA
├─ Recibe paquete (OP_MEM_FIN_PROCESO)
├─ Delega: memoria_adapter_fin_proceso(req, fd)
│   ├─ Llama: paginacion_destruir_proceso(pid)
│   └─ No responde
└─ Continúa
```

**Estado: ✅ CORRECTO**
- One-way, no necesita respuesta

---

### 3. CPU ↔ MEMORIA

#### Flujo: CPU → Memoria (FETCH INSTRUCCIÓN)

```
CPU (Ciclo de Instrucción)
├─ Necesita: instrucción en PC
└─ Llama: cpu_memoria_adapter_fetch_instruccion(pid, pc)
    ├─ Prepara: t_mem_fetch
    ├─ Envía: enviar_fetch_instruccion(fd_memoria, req, OP_MEM_FETCH_INSTRUCCION)
    ├─ Espera: t_paquete* resp = recibir_paquete(fd_memoria)
    ├─ Deserializa: char* instr = paquete_read_string(resp)
    └─ Retorna instrucción

    ↓ RED ↓

MEMORIA
├─ Recibe paquete (OP_MEM_FETCH_INSTRUCCION)
├─ Delega: memoria_adapter_fetch_instruccion(req, fd)
│   ├─ Llama: paginacion_leer_instruccion(pid, pc)
│   └─ Envía: enviar_respuesta_instruccion(fd, instr)
└─ Continúa

CPU
├─ Procesa instrucción
└─ Continúa ciclo
```

**Estado: ✅ CORRECTO**
- Request-Response síncrono
- CPU bloquea esperando respuesta
- Memoria envía instrucción

---

#### Flujo: CPU → Memoria (TRADUCIR PÁGINA)

```
CPU (Durante ejecución)
├─ Necesita: Traducción dirección lógica → física
└─ Llama: cpu_memoria_adapter_traducir_pagina(pid, pagina, &marco)
    ├─ Prepara: t_mem_traducir
    ├─ Envía: enviar_traduccion_pagina(fd, req, OP_MEM_TRADUCIR_PAGINA)
    ├─ Espera: t_paquete* resp = recibir_paquete(fd)
    ├─ Deserializa: t_mem_respuesta_traduccion = recibir_respuesta_traduccion(resp)
    ├─ Extrae marco y éxito
    └─ Retorna bool (true si éxito)

    ↓ RED ↓

MEMORIA
├─ Recibe paquete (OP_MEM_TRADUCIR_PAGINA)
├─ Delega: memoria_adapter_traducir_pagina(req, fd)
│   ├─ Obtiene: t_pagina* = paginacion_obtener_entrada(pid, pagina)
│   ├─ Si NO presente: asigna marco (page fault handling)
│   └─ Envía: enviar_respuesta_traduccion(fd, &resp)
└─ Continúa

CPU
├─ Obtiene marco
└─ Calcula dirección física = marco * TAM_PAG + offset
```

**Estado: ✅ CORRECTO**
- Request-Response síncrono
- Memoria resuelve page faults
- CPU obtiene marco traducido

---

#### Flujo: CPU → Memoria (LEER)

```
CPU
└─ Llama: cpu_memoria_adapter_leer(pid, dir_fisica, buffer, size)
    ├─ Prepara: t_mem_read
    ├─ Envía: enviar_lectura_memoria(fd, req, OP_MEM_LEER)
    ├─ Espera: t_paquete* resp
    ├─ Deserializa: t_mem_respuesta_lectura
    ├─ memcpy(buffer, resp->data, size)
    └─ Retorna bool

    ↓ RED ↓

MEMORIA
├─ Recibe paquete (OP_MEM_LEER)
├─ Delega: memoria_adapter_leer(req, fd)
│   ├─ Traduce: dir_logica → dir_fisica
│   ├─ Llama: leer_memoria_fisica(dir_fisica, buffer, size)
│   └─ Envía: enviar_respuesta_lectura(fd, &resp)
└─ Continúa

CPU
├─ Obtiene datos en buffer
└─ Continúa
```

**Estado: ✅ CORRECTO**
- Request-Response síncrono
- Memoria maneja traducción interna
- CPU obtiene datos leídos

---

#### Flujo: CPU → Memoria (ESCRIBIR)

```
CPU
└─ Llama: cpu_memoria_adapter_escribir(pid, dir_fisica, buffer, size)
    ├─ Prepara: t_mem_write (sin copiar buffer)
    ├─ Envía: enviar_escritura_memoria(fd, &req, OP_MEM_ESCRIBIR)
    ├─ Espera: int respuesta = recv(fd, ...)
    └─ Retorna bool (respuesta == 1)

    ↓ RED ↓

MEMORIA
├─ Recibe paquete (OP_MEM_ESCRIBIR)
├─ Delega: memoria_adapter_escribir(req, fd)
│   ├─ Traduce: dir_logica → dir_fisica
│   ├─ Llama: escribir_memoria_fisica(dir_fisica, buffer, size)
│   ├─ Marca: pag->modificado = true
│   └─ Envía: enviar_respuesta_ok(fd) o enviar_respuesta_fail(fd)
└─ Continúa

CPU
├─ Valida escritura
└─ Continúa
```

**Estado: ✅ CORRECTO**
- Request-Response síncrono
- Memoria maneja dirección física
- Confirmación simple (int)

---

### 4. KERNEL ↔ IO

#### Flujo: Kernel → IO (SLEEP)

```
KERNEL (manejar_bloqueo_io)
├─ PCB solicita SLEEP
└─ Llama: kernel_io_adapter_sleep(pcb, tiempo_ms, "INTERFAZ_NAME")
    ├─ Prepara: t_io_sleep
    ├─ Obtiene: socket_io = obtener_socket_interfaz("INTERFAZ_NAME")
    ├─ Envía: enviar_io_sleep(socket_io, req)
    └─ Retorna (no espera respuesta, proceso bloqueado en Kernel)

    ↓ RED ↓

IO (Servidor)
├─ Recibe paquete (OP_IO_SLEEP)
├─ sleep(tiempo_ms)
└─ Envía: OP_IO_FIN_OPERACION con PID

    ↓ RED ↓

KERNEL (Servidor IO Listener)
├─ Recibe: OP_IO_FIN_OPERACION
├─ Extrae: PID
└─ Desbloquea: agrega PCB a cola de READY
```

**Estado: ✅ CORRECTO (en concepto)**
- Adapter prepara request
- IO ejecuta operación
- Respuesta es notificación de fin
- Kernel desbloquea proceso

**NOTA:** kernel_io_adapter.c necesita definir `obtener_socket_interfaz()`

---

#### Flujo: Kernel → IO (FS)

```
KERNEL
└─ Llama: kernel_io_adapter_fs_operation(pcb, "CREATE", "archivo.txt", 0, "IO_FS")
    ├─ Obtiene: socket_io
    ├─ Switch tipo_operacion: CREATE, DELETE, READ, WRITE, TRUNCATE
    ├─ Prepara estructura (t_io_fs_create, t_io_fs_write, etc.)
    ├─ Envía: enviar_io_fs_create(socket_io, req) [u otro]
    └─ Retorna (no espera)

IO
├─ Ejecuta operación FS
└─ Notifica fin

KERNEL
├─ Recibe: OP_IO_FIN_OPERACION
└─ Desbloquea PCB
```

**Estado: ⚠️ PARCIAL**
- Estructura es correcta
- Falta implementar `enviar_io_fs_delete()`, `enviar_io_fs_truncate()`, etc. en protocolo/mensajes

---

## 📋 Resumen de Verificación

### ✅ Implementado Correctamente:

1. **Kernel Memoria Adapter**
   - ✅ kernel_memoria_adapter_init_proceso() - COMPLETO
   - ✅ kernel_memoria_adapter_fin_proceso() - COMPLETO

2. **Kernel CPU Adapter**
   - ✅ kernel_cpu_adapter_dispatch() - COMPLETO
   - ✅ kernel_cpu_adapter_interrupt() - COMPLETO

3. **CPU Memoria Adapter**
   - ✅ cpu_memoria_adapter_fetch_instruccion() - COMPLETO
   - ✅ cpu_memoria_adapter_traducir_pagina() - COMPLETO
   - ✅ cpu_memoria_adapter_leer() - COMPLETO
   - ✅ cpu_memoria_adapter_escribir() - COMPLETO

4. **Memoria Adapter**
   - ✅ memoria_adapter_init_proceso() - COMPLETO
   - ✅ memoria_adapter_fin_proceso() - COMPLETO
   - ✅ memoria_adapter_traducir_pagina() - COMPLETO
   - ✅ memoria_adapter_fetch_instruccion() - COMPLETO
   - ✅ memoria_adapter_leer() - COMPLETO
   - ✅ memoria_adapter_escribir() - COMPLETO

5. **Servidores**
   - ✅ Memoria server - DELEGADO A ADAPTADORES
   - ✅ CPU server - DELEGA A CICLO

### ⚠️ Pendiente:

1. **kernel_io_adapter.c**
   - ⚠️ Necesita `obtener_socket_interfaz()` - EXTERNA
   - ⚠️ FS operations parciales (falta TRUNCATE, DELETE completos)

2. **protocolo/mensajes.h/c**
   - ⚠️ Falta: `enviar_io_fs_delete()`
   - ⚠️ Falta: `enviar_io_fs_truncate()`
   - ⚠️ Falta: `enviar_io_fs_read()`

3. **IO Server**
   - ⚠️ No revisado en detalle
   - ⚠️ Debe delegar a adaptadores (como Memoria)

---

## 🎯 Próximos Pasos

1. Completar `protocolo/mensajes.h/c` con operaciones IO faltantes
2. Revisar y refactorizar IO adapter
3. Crear IO adapter similar al de Memoria
4. Refactorizar servidor IO
5. Validar flujo completo leyendo código (sin make)

