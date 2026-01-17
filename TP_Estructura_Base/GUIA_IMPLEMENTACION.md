# Guía de Implementación - Arquitectura de Comunicación

## 📊 Estado Actual del Refactoring

### ✅ Completado

#### Adaptadores Kernel
- [x] `kernel_memoria_adapter.h/c` - REFACTORIZADO
  - `kernel_memoria_adapter_init_proceso()` - operación completa
  - `kernel_memoria_adapter_fin_proceso()` - operación completa
- [x] `pcb_cpu_adapter.h/c` - REFACTORIZADO
  - `kernel_cpu_adapter_dispatch()` - operación completa
  - `kernel_cpu_adapter_interrupt()` - operación completa
- [x] `kernel_io_adapter.h/c` - REFACTORIZADO
  - `kernel_io_adapter_sleep()` - operación completa
  - `kernel_io_adapter_fs_operation()` - parcial (CREATE, WRITE operacionales)

#### Adaptadores CPU
- [x] `cpu_memoria_adapter.h/c` - REFACTORIZADO
  - `cpu_memoria_adapter_fetch_instruccion()` - operación completa
  - `cpu_memoria_adapter_traducir_pagina()` - operación completa
  - `cpu_memoria_adapter_leer()` - operación completa
  - `cpu_memoria_adapter_escribir()` - operación completa
- [x] `contexto_cpu_adapter.h/c` - REFACTORIZADO
  - `cpu_contexto_adapter_cargar()` - operación completa
  - `cpu_contexto_adapter_extraer()` - operación completa

#### Adaptadores Memoria
- [x] `memoria_adapter.h/c` - REFACTORIZADO
  - `memoria_adapter_init_proceso()` - operación completa + respuesta
  - `memoria_adapter_fin_proceso()` - operación completa
  - `memoria_adapter_traducir_pagina()` - operación completa + respuesta
  - `memoria_adapter_fetch_instruccion()` - operación completa + respuesta
  - `memoria_adapter_leer()` - operación completa + respuesta
  - `memoria_adapter_escribir()` - operación completa + respuesta

#### Servidores
- [x] `memoria/server/server.c` - REFACTORIZADO
  - Servidor SOLO recibe y delega a adaptadores
  - Todos los casos (OP_*) delegados a adaptadores

- [x] `kernel/conexiones/io.c` - MEJORADO
  - Agregada función `obtener_socket_interfaz()`
  - Mejorado handler para notificaciones de fin

#### Documentación
- [x] `ARQUITECTURA_COMUNICACION.md` - Documento de arquitectura detallado
- [x] `VALIDACION_FLUJOS.md` - Validación de flujos de comunicación

---

## ⚠️ Pendiente de Implementación/Revisión

### Protocolo/Mensajes
- [ ] `enviar_io_fs_delete()` y `recibir_io_fs_delete()` - FALTA
- [ ] `enviar_io_fs_read()` y `recibir_io_fs_read()` - FALTA
- [ ] `enviar_io_fs_truncate()` y `recibir_io_fs_truncate()` - FALTA
- [ ] Validar que `enviar_respuesta_ok/fail()` existan - REVISAR

### Kernel
- [ ] `conexiones/memoria.c` - ¿Aún tiene lógica antigua? REVISAR
- [ ] `conexiones/cpu.c` - Validar que usa adaptadores correctamente
- [ ] Función manejar_fin_io_operacion() - FALTA (desbloquear proceso)

### CPU
- [ ] `conexiones/cpu_memoria.c` - ¿Aún tiene código duplicado? REVISAR
- [ ] Inicializar `cpu_memoria_adapter_init()` en main.c

### IO
- [ ] Servidor IO - NO REVISADO AÚN
- [ ] Crear IO adapter si es necesario

---

## 🎯 Checklist de Validación por Flujo

### Flujo 1: Kernel → Memoria (INIT_PROCESO)

**Código a revisar:**
```
1. kernel/src/peticiones/proceso.c
   └─ crear_nuevo_proceso() 
      └─ kernel_memoria_adapter_init_proceso(pcb)
   
2. kernel/src/adaptadores/kernel_memoria_adapter.c
   └─ kernel_memoria_adapter_init_proceso()
      ├─ pcb_a_mem_init(pcb)
      ├─ enviar_init_proceso(socket_memoria, req, OP_MEM_INIT_PROCESO)
      ├─ recv(socket_memoria, &respuesta, ...)
      └─ return bool
   
3. utils/protocolo/mensajes.h/c
   ├─ enviar_init_proceso()
   │  └─ serializar_mem_init_proceso()
   │     └─ enviar_paquete()
   └─ recibir_init_proceso()
      └─ deserializar_mem_init_proceso()
   
4. memoria/server/server.c
   ├─ recibe OP_MEM_INIT_PROCESO
   ├─ recibir_init_proceso(paquete)
   └─ memoria_adapter_init_proceso(req, fd)
      ├─ paginacion_crear_proceso()
      └─ enviar_respuesta_ok(fd) o fail
```

**Validar:**
- ✅ Adaptador existe y está completo
- ✅ Protocolo tiene enviar/recibir
- ✅ Servidor delega a adaptador
- ✅ Respuesta bloqueante en Kernel
- ✅ Adaptador maneja respuesta

**Estado: IMPLEMENTADO**

---

### Flujo 2: CPU → Memoria (FETCH INSTRUCCIÓN)

**Código a revisar:**
```
1. cpu/src/ciclo_instruccion/ciclo.c
   └─ ciclo_instruccion_ejecutar()
      └─ memoria_fetch_instruccion(pid, pc)
         └─ cpu_memoria_adapter_fetch_instruccion(pid, pc)
   
2. cpu/src/adaptadores/cpu_memoria_adapter.c
   └─ cpu_memoria_adapter_fetch_instruccion()
      ├─ cpu_a_mem_fetch()
      ├─ enviar_fetch_instruccion()
      ├─ recibir_paquete()
      ├─ deserializar instrucción
      └─ return char*
   
3. memoria/server/server.c
   ├─ recibe OP_MEM_FETCH_INSTRUCCION
   ├─ recibir_fetch(paquete)
   └─ memoria_adapter_fetch_instruccion(req, fd)
      ├─ paginacion_leer_instruccion()
      └─ enviar_respuesta_instruccion(fd, instruccion)
```

**Validar:**
- ✅ Adaptador existe y está completo
- ✅ Protocolo tiene enviar/recibir
- ✅ Servidor delega a adaptador
- ✅ Respuesta bloqueante en CPU
- ✅ Adaptador serializa respuesta

**Estado: IMPLEMENTADO**

---

### Flujo 3: Kernel → IO (SLEEP)

**Código a revisar:**
```
1. kernel/src/peticiones/operaciones.c
   └─ manejar_sleep()
      └─ kernel_io_adapter_sleep(pcb, tiempo, "INTERFAZ")
   
2. kernel/src/adaptadores/kernel_io_adapter.c
   └─ kernel_io_adapter_sleep()
      ├─ pcb_a_io_sleep()
      ├─ obtener_socket_interfaz()
      ├─ enviar_io_sleep()
      └─ return
   
3. kernel/src/conexiones/io.c
   ├─ obtener_socket_interfaz() - NUEVA FUNCIÓN
   └─ lista_interfaces management
   
4. io/src/main.c
   ├─ recibe OP_IO_SLEEP
   └─ sleep(tiempo)
      └─ enviar_io_fin()
   
5. kernel/src/conexiones/io.c
   ├─ recibe OP_IO_FIN_OPERACION
   └─ desbloquea proceso (TODO: implementar)
```

**Validar:**
- ✅ Adaptador existe y está completo
- ✅ obtener_socket_interfaz() implementada
- ⚠️ Falta manejar_fin_io_operacion() en kernel
- ⚠️ Falta verificar que IO envía FIN correctamente

**Estado: 80% IMPLEMENTADO**

---

## 📋 Protocolo/Mensajes - Completitud

### Enviadores Existentes
```
✅ enviar_contexto()
✅ enviar_interrupcion_cpu()
✅ enviar_fetch_instruccion()
✅ enviar_init_proceso()
✅ enviar_fin_proceso()
✅ enviar_traduccion_pagina()
✅ enviar_lectura_memoria()
✅ enviar_escritura_memoria()
✅ enviar_respuesta_lectura()
✅ enviar_respuesta_traduccion()
✅ enviar_respuesta_instruccion()
✅ enviar_io_sleep()
✅ enviar_io_fs_write()
✅ enviar_io_fs_create()
✅ enviar_io_fin()
```

### Receptores Existentes
```
✅ recibir_contexto()
✅ recibir_fetch()
✅ recibir_init_proceso()
✅ recibir_fin_proceso()
✅ recibir_mem_traducir_pagina()
✅ recibir_lectura_memoria()
✅ recibir_escritura_memoria()
✅ recibir_respuesta_lectura()
✅ recibir_respuesta_traduccion()
✅ recibir_respuesta_instruccion()
✅ recibir_io_sleep()
✅ recibir_io_fs_write()
✅ recibir_io_fs_create()
✅ recibir_pid_fin_io()
```

### Faltantes (Bajo Prioridad)
```
❌ enviar_io_fs_delete()
❌ recibir_io_fs_delete()
❌ enviar_io_fs_read()
❌ recibir_io_fs_read()
❌ enviar_io_fs_truncate()
❌ recibir_io_fs_truncate()
```

---

## 🚀 Próximos Pasos (Orden de Prioridad)

### CRÍTICO (Bloquea ejecución)

1. **Validar kernell/conexiones/memoria.c**
   - ¿Aún tiene lógica de envío directa?
   - ¿O ya usa kernel_memoria_adapter completamente?
   - **Acción:** Revisar y eliminar código duplicado

2. **Validar cpu/conexiones/cpu_memoria.c**
   - ¿Aún tiene lógica de request-response?
   - ¿O ya delega todo a adaptadores?
   - **Acción:** Revisar y referencias

3. **Implementar obtener_socket_interfaz() en kernel/conexiones/io.c**
   - ✅ YA IMPLEMENTADO
   - **Acción:** Ninguna

4. **Inicializar cpu_memoria_adapter en cpu/src/main.c**
   - **Acción:** Llamar `cpu_memoria_adapter_init(fd_memoria)` después de conectar

### IMPORTANTE (Falta para funcionalidad)

5. **Implementar IO Adapter** (si no existe)
   - Revisar estructura de IO
   - Crear adapter si es necesario
   - Refactorizar servidor IO

6. **Implementar manejar_fin_io_operacion() en kernel**
   - Recibir PID
   - Buscar proceso
   - Mover a cola READY
   - **Ubicación:** kernel/src/peticiones/operaciones.c o kernel/src/conexiones/io.c

7. **Completar FS operations en protocolo**
   - enviar/recibir_io_fs_delete()
   - enviar/recibir_io_fs_read()
   - enviar/recibir_io_fs_truncate()
   - **Ubicación:** utils/src/protocolo/mensajes.h/c

### VALIDACIÓN (Verificación)

8. **Prueba de flujo: Kernel → Memoria (INIT)**
   - Leer código end-to-end
   - Validar que no hay duplicación
   - Validar que todos usan protocolo

9. **Prueba de flujo: CPU → Memoria (FETCH)**
   - Verificar request-response síncrono
   - Validar manejo de errores

10. **Prueba de flujo: Kernel → IO (SLEEP)**
    - Verificar async + callback
    - Validar desbloqueo de proceso

---

## 📍 Archivos Críticos a Revisar

```
REVISAR:
├── kernel/src/conexiones/memoria.c
│   └─ ¿Duplica lógica de adapter?
├── kernel/src/conexiones/cpu.c
│   └─ ¿Usa adapter o código antiguo?
├── kernel/src/peticiones/proceso.c
│   └─ ¿Llama a adapters?
├── cpu/src/ciclo_instruccion/ciclo.c
│   └─ ¿Llama a adapters?
├── cpu/src/main.c
│   └─ ¿Inicializa adapters?
├── entradasalida/src/core/io_main.c
│   └─ ¿Estructura correcta?
└── utils/src/protocolo/mensajes.c
    └─ ¿Tiene todos los enviar/recibir?
```

---

## 💡 Conceptos Clave Implementados

### 1. Separación de Responsabilidades
- **Servidor:** SOLO recibe paquetes y delega
- **Adaptador:** Lógica de transformación + envío de respuestas
- **Protocolo:** Serialización/deserialización
- **Lógica interna:** Operaciones del módulo sin conocimiento de red

### 2. Request-Response Patterns
- **Síncrono:** CPU ↔ Memoria (bloquea esperando respuesta)
- **Síncrono:** Kernel ↔ Memoria (bloquea esperando OK)
- **Asíncrono:** Kernel → IO (non-blocking, callback via FIN_OPERACION)

### 3. Flujos de Información
- Adaptadores siempre usan protoc olo/mensajes para enviar
- Adaptadores TRANSFORMAN entre externo (compartido) e interno (módulo)
- Servidores solo leen op_code y llaman adaptador

---

## ✨ Beneficios de la Arquitectura

1. **Claridad:** Cada capa tiene responsabilidad única
2. **Mantenibilidad:** Cambios de protocolo solo afectan protocolo/adaptador
3. **Testabilidad:** Adaptadores pueden testearse sin red
4. **Escalabilidad:** Nuevas operaciones = nuevo adaptador
5. **Reutilización:** Adaptadores usados por lógica interna y servidor

---

## 📞 Soporte

Si durante implementación encuentras:
- **Inconsistencias de structs:** Revisar `utils/common/`
- **Errores de op_code:** Revisar `utils/protocolo/op_code.h`
- **Falta de serialización:** Revisar `utils/protocolo/mensajes.h/c`
- **Confusión de flujo:** Consultar `ARQUITECTURA_COMUNICACION.md` o `VALIDACION_FLUJOS.md`

