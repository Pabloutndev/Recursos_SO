# CHECKLIST PRE-COMPILACIÓN

## 🔍 Validación de Arquitectura

Antes de ejecutar `make`, revisa estos puntos para asegurar que todo está consistente:

---

## ✅ Kernel - Adaptadores

- [ ] `kernel_memoria_adapter.c` importa:
  - [ ] `#include <protocolo/mensajes.h>`
  - [ ] `#include <protocolo/op_code.h>`
  - [ ] `extern int socket_memoria;`
  - [ ] `extern t_log* logger;`

- [ ] Función `kernel_memoria_adapter_init_proceso()`:
  - [ ] Convierte PCB con `pcb_a_mem_init()`
  - [ ] Envía con `enviar_init_proceso()`
  - [ ] Espera respuesta con `recv(socket_memoria, ...)`
  - [ ] Retorna bool

- [ ] Función `kernel_memoria_adapter_fin_proceso()`:
  - [ ] Prepara estructura
  - [ ] Envía con `enviar_fin_proceso()`
  - [ ] No espera respuesta (one-way)

---

- [ ] `pcb_cpu_adapter.c` tiene:
  - [ ] Función `kernel_cpu_adapter_dispatch(t_pcb* pcb)`
  - [ ] Función `kernel_cpu_adapter_interrupt()`
  - [ ] Usa `enviar_contexto()` de protocolo

---

- [ ] `kernel_io_adapter.c` tiene:
  - [ ] `extern int obtener_socket_interfaz(...);` (forward declaration)
  - [ ] Función `kernel_io_adapter_sleep()`
  - [ ] Función `kernel_io_adapter_fs_operation()`
  - [ ] Ambas usan `obtener_socket_interfaz()` para obtener socket

---

## ✅ Kernel - Conexiones

- [ ] `kernel/conexiones/io.c` define:
  - [ ] Struct `typedef struct { char* nombre; int socket; } t_interfaz_io;`
  - [ ] `static t_list* lista_interfaces;`
  - [ ] Función `int obtener_socket_interfaz(const char* nombre_interfaz)`
  - [ ] Función `static bool _find_interfaz_por_nombre(...)`

- [ ] `obtener_socket_interfaz()`:
  - [ ] Busca en lista_interfaces
  - [ ] Retorna socket si encuentra
  - [ ] Retorna -1 si no encuentra
  - [ ] Log info/warning apropiados

---

## ✅ CPU - Adaptadores

- [ ] `cpu_memoria_adapter.h` declara:
  - [ ] `t_mem_fetch* cpu_a_mem_fetch(...)`
  - [ ] `t_mem_traducir* cpu_a_mem_traduccion(...)`
  - [ ] `cpu_memoria_adapter_fetch_instruccion()`
  - [ ] `cpu_memoria_adapter_traducir_pagina()`
  - [ ] `cpu_memoria_adapter_leer()`
  - [ ] `cpu_memoria_adapter_escribir()`

- [ ] `cpu_memoria_adapter.c` implementa:
  - [ ] `int cpu_memoria_adapter_init(int fd_mem)` - inicializa fd_memoria
  - [ ] Todas las funciones declaradas
  - [ ] Cada una hace request-response completo
  - [ ] Usa `recibir_paquete()` para respuestas
  - [ ] Deserializa correctamente

---

- [ ] `contexto_cpu_adapter.c`:
  - [ ] `void cpu_contexto_adapter_cargar(t_contexto_cpu* ctx)` - carga en cpu_estado
  - [ ] `t_contexto_cpu* cpu_contexto_adapter_extraer(void)` - extrae de cpu_estado

---

## ✅ Memoria - Adaptadores

- [ ] `memoria_adapter.h` declara:
  - [ ] `memoria_adapter_init_proceso(req, socket_kernel)`
  - [ ] `memoria_adapter_fin_proceso(req, socket_kernel)`
  - [ ] `memoria_adapter_traducir_pagina(req, socket_cpu)`
  - [ ] `memoria_adapter_fetch_instruccion(req, socket_cpu)`
  - [ ] `memoria_adapter_leer(req, socket_cpu)`
  - [ ] `memoria_adapter_escribir(req, socket_cpu)`

- [ ] `memoria_adapter.c` implementa todas con:
  - [ ] Validación de entrada
  - [ ] Llamadas a lógica interna (paginacion_*, etc.)
  - [ ] Serialización de respuesta con protocolo
  - [ ] `send()` directo para respuestas simples (int)
  - [ ] `enviar_respuesta_*()` para respuestas complejas

---

## ✅ Memoria - Servidor

- [ ] `memoria/server/server.c`:
  - [ ] Importa `#include <adaptadores/memoria_adapter.h>`
  - [ ] `handler_dispatch()` NO tiene lógica, SOLO delega
  - [ ] Cada caso (OP_*) llama a `memoria_adapter_*()`
  - [ ] Limpia estructuras después de procesar

---

## ✅ Protocolo/Mensajes

- [ ] `protocolo/mensajes.h` declara:
  - [ ] `void enviar_contexto()`
  - [ ] `t_contexto_cpu* recibir_contexto()`
  - [ ] `void enviar_init_proceso()`
  - [ ] `t_mem_init_proceso* recibir_init_proceso()`
  - [ ] `void enviar_traduccion_pagina()`
  - [ ] `t_mem_respuesta_traduccion* recibir_respuesta_traduccion()`
  - [ ] Y así para TODOS los tipos

- [ ] `protocolo/mensajes.c` implementa:
  - [ ] Cada `enviar_*()` hace:
    ```c
    t_paquete* p = paquete_create(code);
    paquete_write_*(...);  // Serializa campos
    enviar_paquete(socket, p);
    paquete_destroy(p);
    ```
  - [ ] Cada `recibir_*()` hace:
    ```c
    t_struct* req = malloc(...);
    paquete_read_*(...);  // Deserializa campos
    return req;
    ```

---

## ✅ Estructuras Compartidas

- [ ] `utils/common/cpu/contexto.h`:
  - [ ] `typedef struct { uint32_t pid, pc, quantum; ... } t_contexto_cpu;`

- [ ] `utils/common/memoria/memoria.h`:
  - [ ] `typedef struct { uint32_t pid, tamanio; ... } t_mem_init_proceso;`
  - [ ] `typedef struct { uint32_t pid; } t_mem_fin_proceso;`
  - [ ] `typedef struct { uint32_t pid, direccion_logica; } t_mem_traducir;`
  - [ ] Y otros...

- [ ] `utils/common/io/io_ops.h`:
  - [ ] `typedef struct { uint32_t pid, tiempo; } t_io_sleep;`
  - [ ] `typedef struct { uint32_t pid; char* nombre_archivo; } t_io_fs_create;`
  - [ ] Y otros...

---

## ✅ Op Codes

- [ ] `protocolo/op_code.h` define:
  - [ ] `OP_PROCESO_EXEC` (200)
  - [ ] `OP_MEM_INIT_PROCESO` (300)
  - [ ] `OP_MEM_FIN_PROCESO` (301)
  - [ ] `OP_MEM_TRADUCIR_PAGINA` (302)
  - [ ] `OP_MEM_RESP_TRADUCCION` (312)
  - [ ] Y todos los demás necesarios

---

## ✅ Compilación

- [ ] No hay ciclos de includes:
  - [ ] Adaptadores ← Protocolo ✅ (correcto)
  - [ ] Protocolo NO ← Adaptadores ✅ (correcto)
  - [ ] Servidor ← Adaptadores ✅ (correcto)
  - [ ] Adaptadores NO ← Servidor ✅ (correcto)

- [ ] Forward declarations en lugar de includes donde sea necesario:
  - [ ] `extern int socket_memoria;` en adaptadores ✅
  - [ ] `extern int obtener_socket_interfaz(...);` en kernel_io_adapter ✅

- [ ] No hay referencias circulares

---

## 🔴 CRÍTICO - Sin esto NO compilará

1. **kernel_io_adapter.c necesita `obtener_socket_interfaz()` definida**
   - [ ] Existe en `kernel/conexiones/io.c`
   - [ ] Se declara como `extern int obtener_socket_interfaz(...);` en .h

2. **cpu_memoria_adapter.c necesita inicialización**
   - [ ] `main.c` de CPU debe llamar: `cpu_memoria_adapter_init(fd_memoria);`

3. **memoria/server/server.c necesita `#include <adaptadores/memoria_adapter.h>`**
   - [ ] Si no, no conoce funciones para delegar

---

## 🟡 ADVERTENCIAS - Podrían causar problemas

- [ ] Si hay llamadas a funciones antiguas en conexiones/memoria.c
  - **Acción:** Revisar y eliminar código duplicado

- [ ] Si hay referencias directas a `paquete_*` en servidores
  - **Acción:** Debe delegarse a adaptador

- [ ] Si adaptadores llaman directamente a `paquete_create/destroy`
  - **Acción:** Debe usar funciones de protocolo/mensajes

---

## ✨ Extras (Nice to Have, No Bloquea)

- [ ] Nombres consistentes en logs (ADAPTER:, SERVIDOR:, etc.)
- [ ] Todos los errores tienen `log_error()` o `log_warning()`
- [ ] Respuestas siempre van al socket correcto (del cliente)
- [ ] No hay memory leaks en adaptadores (free después de use)

---

## 📝 Notas Importantes

### Flujo de Kernel → Memoria Init:
```
kernel/peticiones/proceso.c
  └─ crear_proceso()
     └─ kernel_memoria_adapter_init_proceso(pcb)
        ├─ enviar_init_proceso()
        ├─ recv(respuesta)
        └─ return bool

memoria/server/server.c
  └─ OP_MEM_INIT_PROCESO
     └─ memoria_adapter_init_proceso(req, fd)
        ├─ paginacion_crear_proceso()
        └─ enviar_respuesta_ok/fail()
```

### Flujo de CPU → Memoria Fetch:
```
cpu/ciclo_instruccion/ciclo.c
  └─ cpu_memoria_adapter_fetch_instruccion()
     ├─ enviar_fetch_instruccion()
     ├─ recibir_paquete()
     └─ return instruccion

memoria/server/server.c
  └─ OP_MEM_FETCH_INSTRUCCION
     └─ memoria_adapter_fetch_instruccion(req, fd)
        ├─ paginacion_leer_instruccion()
        └─ enviar_respuesta_instruccion()
```

---

## ✅ Validación Final

Marca aquí cuando hayas completado:

- [ ] Revisé todos los adaptadores
- [ ] Revisé todos los servidores (solo delegan)
- [ ] Revisé protocolo/mensajes (completo)
- [ ] Revisé estructuras compartidas (coherentes)
- [ ] Revisé op_codes (definidos)
- [ ] Revisé includes (no hay ciclos)
- [ ] Compilé mentalmente (no hay errores obvios)
- [ ] Validé flujos end-to-end (sin duplicación)

**Si marcaste TODO ✅, estás listo para `make`!**

---

## 🆘 Si algo falla:

1. **Error de símbolo no encontrado:** Probablemente falta `#include` o función no implementada
   - Revisar documentación de símbolos faltantes
   - Ver GUIA_IMPLEMENTACION.md

2. **Error de tipo:** Estructura no coincide
   - Revisar en `utils/common/`
   - Validar protocolo/mensajes

3. **Error en enlazado:** Implementación no coincide con declaración
   - Revisar headers de adaptadores
   - Validar nombres y firmas

4. **Segmentation Fault:**  (después de compilar)
   - Revisar manejo de sockets
   - Validar inicializaciones
   - Revisar free/malloc

