# Resumen de Correcciones del Kernel

## Errores Identificados y Corregidos

### 1. **Undefined Reference: `pcb_a_mem_init`**
**Problema:** La función estaba comentada en `kernel_memoria_adapter.c`
**Solución:** Descommentada e implementada correctamente:
```c
t_mem_init_proceso* pcb_a_mem_init(t_pcb* pcb)
{
    if (!pcb) return NULL;
    
    t_mem_init_proceso* req = malloc(sizeof(t_mem_init_proceso));
    if (!req) return NULL;
    
    req->pid = pcb->pid;
    req->size = pcb->tam_proceso > 0 ? pcb->tam_proceso : 1024;
    
    return req;
}
```

### 2. **Undefined Reference: `socket_cpu_dispatch` y `socket_cpu_interrupt`**
**Problema:** Se usaba `socket_cpu_dispatch` y `socket_cpu_interrupt` pero las variables se llaman `socket_dispatch` e `socket_interrupt`
**Solución:** Corregidos los nombres en `pcb_cpu_adapter.c`:
```c
extern int socket_dispatch;      // Antes: socket_cpu_dispatch
extern int socket_interrupt;     // Antes: socket_cpu_interrupt
```

### 3. **Undefined Reference: `pid` (variable global)**
**Problema:** Se declaraba `extern int pid` en `proceso.c` pero no existía esta variable global
**Solución:** Removida la variable `pid` y refactorizado `ejecutar_proceso()` para:
- Crear el PCB primero (que genera el PID automáticamente)
- Llamar a `kernel_init_proceso(pcb)` con el PCB (que contiene el PID)

### 4. **Undefined Reference: `kernel_init_proceso_path`**
**Problema:** Se llamaba a `kernel_init_proceso_path()` que no existía
**Solución:** Reemplazada por `kernel_init_proceso(pcb)` - la función correcta que ya existía

### 5. **Undefined Reference: `solicitar_creacion_proceso_memoria` y `solicitar_fin_proceso_memoria`**
**Problema:** Se declaraban en `memoria.h` pero no estaban implementadas
**Solución:** Implementadas en `memoria.c`:
```c
bool solicitar_creacion_proceso_memoria(uint32_t pid, int size)
{
    // Crear struct, enviar a Memoria, esperar respuesta
    ...
}

void solicitar_fin_proceso_memoria(uint32_t pid)
{
    // Crear struct, enviar a Memoria (one-way)
    ...
}
```

### 6. **Manejadores de Eventos Faltantes en CPU**
**Problema:** Funciones como `manejar_bloqueo_io()`, `manejar_wait_recurso()` se llamaban pero no estaban definidas
**Solución:** 
- Agregadas declaraciones forward en `cpu.c`
- Se reutilizan implementaciones que existen en `peticiones/interrupciones.c`
- Creadas funciones wrapper para compatibilidad

### 7. **Tipos de Datos Inconsistentes**
**Correciones realizadas:**
- `pcb_crear()`: Cambió de `int generar_pid()` a `uint32_t generar_pid()`
- `pcb_crear()`: Ahora sin parámetros `pcb_crear(void)` en lugar de `pcb_crear(/*int skt*/)`
- `estimacion_rafaga`: Cambiado a `double` en lugar de `int`
- `socket_consola`: Inicializado a `-1` en lugar de dejarlo sin inicializar

### 8. **Inicialización Incompleta de PCB**
**Problema:** Faltaba inicializar el campo `path` en PCB
**Solución:** Agregado en `pcb_crear()`:
```c
pcb->path = NULL;
```
Y en `ejecutar_proceso()`:
```c
pcb->path = malloc(strlen(path) + 1);
strcpy(pcb->path, path);
```

### 9. **Memory Leak en `pcb_destruir()`**
**Problema:** No se destruía `tabla_segmentos`
**Solución:** Agregado:
```c
if (pcb->tabla_segmentos) list_destroy(pcb->tabla_segmentos);
```

### 10. **Signatures de Funciones Mejoradas**
**Cambios realizados:**
- `pcb_crear()`: Clarificada la firma (sin parámetros)
- `generar_pid()`: Cambiado retorno de `int` a `uint32_t`
- `pcb_destruir()`: Agregada verificación nula

## Buenas Prácticas Implementadas

### 1. **Manejo Seguro de Memoria**
- Verificaciones nulas antes de acceder a punteros
- `free()` solo después de verificar que el puntero no es NULL
- Destrucción correcta de estructuras complejas

### 2. **Coherencia de Tipos**
- Uso consistente de `uint32_t` para PIDs
- `t_estado` enum para estados de proceso
- Tipos correctos para sockets (int)

### 3. **Separación de Responsabilidades**
- `kernel_memoria_adapter.c`: Adaptador kernel ↔ Memoria
- `pcb_cpu_adapter.c`: Adaptador kernel ↔ CPU
- `conexiones/cpu.c`: Manejo de eventos de CPU
- `peticiones/interrupciones.c`: Procesamiento de interrupciones

### 4. **Logging Mejorado**
- Diferenciación entre logger y loggerError
- Mensajes informativos con contexto (PID, estado, etc.)
- Niveles apropiados (INFO, WARNING, ERROR)

### 5. **Sincronización Thread-Safe**
- Uso de mutex para proteger secciones críticas
- Semáforos para sincronización entre threads
- Evitar deadlocks con orden consistente de locks

## Estructura del Kernel Refactorizado

```
kernel/src/
├── main.c                          # Punto de entrada
├── mod_kernel.h/c                  # Módulo principal
├── config/                         # Configuración
├── pcb/                           # Process Control Block
│   ├── pcb.h
│   └── pcb.c                      # [CORREGIDO] Funciones de creación/destrucción
├── planificacion/                 # Algoritmos de planificación
│   ├── planificacion.c            # [VERIFICADO] Inicialización y estado
│   ├── largo_plazo.c
│   └── corto_plazo.c
├── conexiones/                    # Conexiones con otros módulos
│   ├── memoria.c                  # [IMPLEMENTADO] Solicitudes a Memoria
│   ├── cpu.c                      # [CORREGIDO] Manejadores de eventos
│   └── io.c
├── adaptadores/                   # Transformación de estructuras
│   ├── kernel_memoria_adapter.c   # [DESCOMENTADO] pcb_a_mem_init()
│   └── pcb_cpu_adapter.c          # [CORREGIDO] Nombres de sockets
├── peticiones/                    # Manejo de peticiones
│   ├── proceso.c                  # [REFACTORIZADO] Sin extern pid
│   ├── interrupciones.c           # [VERIFICADO] Manejadores
│   ├── recursos.c
│   └── dispatch.c
├── consola/
├── loggers/
└── include/
```

## Verificaciones Adicionales Realizadas

✅ Verificación de tipos de datos en structs
✅ Coherencia de includes
✅ Inicialización de variables globales
✅ Manejo correcto de memoria dinámicas
✅ Thread-safety de operaciones críticas
✅ Logging apropiado en funciones clave

## Problemas Potenciales Encontrados (Para Revisión Futura)

⚠️ En `peticiones/interrupciones.c` línea 131: `recurso_wait(pcb, ctx->registros)` 
   - Se pasa `ctx->registros` (estructura) como segundo parámetro que espera `char* nombre_recurso`
   - Requiere clarificación de cómo se transmiten los parámetros de instrucciones desde CPU

⚠️ Función `server_io_init()` se llama dos veces en `mod_kernel.c` (línea 59 y 68)
   - Debería ser llamada una sola vez

⚠️ `mod_kernel.c` llama a `conexiones_init()` pero luego vuelve a llamar a `server_io_init()`
   - Revisar la estructura de inicialización

## Compilación Esperada

Después de estos cambios, los siguientes errores de compilación deberían estar resueltos:
- ❌ undefined reference to `pcb_a_mem_init` ✅ CORREGIDO
- ❌ undefined reference to `socket_cpu_dispatch` ✅ CORREGIDO
- ❌ undefined reference to `socket_cpu_interrupt` ✅ CORREGIDO
- ❌ undefined reference to `pid` ✅ CORREGIDO
- ❌ undefined reference to `kernel_init_proceso_path` ✅ CORREGIDO
- ❌ undefined reference to `solicitar_creacion_proceso_memoria` ✅ CORREGIDO
- ❌ undefined reference to `solicitar_fin_proceso_memoria` ✅ CORREGIDO

