# Dependencias de Includes - TP Sistemas Operativos UTN FRBA

## 📋 Índice
1. [Módulo CPU](#módulo-cpu)
2. [Módulo Memoria](#módulo-memoria)
3. [Módulo FileSystem](#módulo-filesystem)
4. [Dependencias entre Módulos](#dependencias-entre-módulos)

---

## 🖥️ MÓDULO CPU

### `mod_cpu.h`
**Archivo principal del módulo CPU**

```c
#include <stdint.h>
#include <pthread.h>

// Módulos internos
#include <config/cpu_config.h>
#include <conexiones/kernel.h>
#include <conexiones/memoria.h>
#include <ciclo_instruccion/ciclo.h>
#include <instrucciones/instrucciones.h>
#include <registros/registros.h>
#include <mmu/mmu.h>
#include <interrupciones/interrupciones.h>
#include <contexto/contexto.h>
#include <loggers/logger.h>

// Externos
extern t_log* logger;
extern t_cpu_config CPU_CONF;
```

### `config/cpu_config.h`
**Configuración del módulo CPU**

```c
#include <stdint.h>
#include <stdlib.h>

// No incluye otros módulos internos (configuración básica)
```

### `conexiones/kernel.h`
**Conexión con módulo Kernel**

```c
#include <stdint.h>
#include <utils/conexiones.h>
#include <loggers/logger.h>

// Estructuras compartidas (definidas en include/ipc.h o similar)
// No incluye mod_cpu.h (evitar dependencia circular)
```

### `conexiones/memoria.h`
**Conexión con módulo Memoria**

```c
#include <stdint.h>
#include <utils/conexiones.h>
#include <loggers/logger.h>

// Estructuras compartidas para comunicación
```

### `ciclo_instruccion/ciclo.h`
**Ciclo de instrucción**

```c
#include <stdint.h>
#include <stdbool.h>

#include <instrucciones/instrucciones.h>
#include <registros/registros.h>
#include <mmu/mmu.h>
#include <interrupciones/interrupciones.h>
#include <loggers/logger.h>
```

### `instrucciones/instrucciones.h`
**Decodificación de instrucciones**

```c
#include <stdint.h>
#include <stdbool.h>

#include <instrucciones/operaciones.h>
#include <registros/registros.h>
#include <mmu/mmu.h>
```

### `instrucciones/operaciones.h`
**Operaciones aritméticas y lógicas**

```c
#include <stdint.h>

#include <registros/registros.h>
// Operaciones básicas, no necesita otros módulos complejos
```

### `registros/registros.h`
**Gestión de registros**

```c
#include <stdint.h>

// Estructura de contexto de registros
// No incluye otros módulos internos (es básico)
```

### `mmu/mmu.h`
**Memory Management Unit**

```c
#include <stdint.h>
#include <stdbool.h>

#include <conexiones/memoria.h>  // Para solicitar traducciones a Memoria
#include <loggers/logger.h>
```

### `interrupciones/interrupciones.h`
**Manejo de interrupciones**

```c
#include <stdint.h>

#include <conexiones/kernel.h>  // Para enviar interrupciones al kernel
#include <contexto/contexto.h>  // Para guardar contexto al interrumpir
#include <loggers/logger.h>
```

### `contexto/contexto.h`
**Serialización de contexto**

```c
#include <stdint.h>

#include <registros/registros.h>  // El contexto contiene registros
// Puede incluir estructuras compartidas si el contexto incluye más info
```

### `loggers/logger.h`
**Wrappers de logging para CPU**

```c
#include <commons/log.h>

// No incluye otros módulos internos (solo logging)
```

---

## 💾 MÓDULO MEMORIA

### `mod_memoria.h`
**Archivo principal del módulo Memoria**

```c
#include <stdint.h>
#include <pthread.h>

// Módulos internos
#include <config/memoria_config.h>
#include <conexiones/cpu.h>
#include <conexiones/kernel.h>
#include <conexiones/filesystem.h>
#include <gestion/gestion.h>
#include <segmentacion/segmentacion.h>
#include <paginacion/paginacion.h>
#include <swapping/swapping.h>
#include <marcos/marcos.h>
#include <procesos/procesos.h>
#include <instrucciones/instrucciones.h>
#include <tlb/tlb.h>
#include <loggers/logger.h>

// Externos
extern t_log* logger;
extern t_memoria_config MEM_CONF;
```

### `config/memoria_config.h`
**Configuración del módulo Memoria**

```c
#include <stdint.h>
#include <stdlib.h>

// No incluye otros módulos internos
```

### `conexiones/cpu.h`
**Conexión con módulo CPU**

```c
#include <stdint.h>
#include <utils/conexiones.h>
#include <loggers/logger.h>

// Estructuras para comunicación con CPU
```

### `conexiones/kernel.h`
**Conexión con módulo Kernel**

```c
#include <stdint.h>
#include <utils/conexiones.h>
#include <loggers/logger.h>

// Estructuras para comunicación con Kernel
```

### `conexiones/filesystem.h`
**Conexión con módulo FileSystem**

```c
#include <stdint.h>
#include <utils/conexiones.h>
#include <loggers/logger.h>

// Estructuras para comunicación con FileSystem (swapping, dumps)
```

### `gestion/gestion.h`
**Gestión general de memoria**

```c
#include <stdint.h>
#include <stdbool.h>

#include <segmentacion/segmentacion.h>  // Puede usar segmentación
#include <paginacion/paginacion.h>      // O paginación
#include <marcos/marcos.h>              // Gestiona marcos
#include <procesos/procesos.h>          // Procesos en memoria
#include <loggers/logger.h>
```

### `gestion/asignacion.h`
**Algoritmos de asignación**

```c
#include <stdint.h>
#include <stdbool.h>

#include <marcos/marcos.h>  // Para asignar marcos
#include <loggers/logger.h>
```

### `segmentacion/segmentacion.h`
**Implementación de segmentación**

```c
#include <stdint.h>
#include <stdbool.h>

#include <segmentacion/tabla_segmentos.h>  // Usa tabla de segmentos
#include <marcos/marcos.h>                 // Para asignar marcos
#include <loggers/logger.h>
```

### `segmentacion/tabla_segmentos.h`
**Tabla de segmentos**

```c
#include <stdint.h>
#include <commons/collections/list.h>

// Estructura de entrada de tabla de segmentos
// No incluye otros módulos complejos (solo estructuras)
```

### `paginacion/paginacion.h`
**Implementación de paginación**

```c
#include <stdint.h>
#include <stdbool.h>

#include <paginacion/tabla_paginas.h>  // Usa tabla de páginas
#include <marcos/marcos.h>             // Para asignar marcos
#include <tlb/tlb.h>                   // Usa TLB para acelerar
#include <swapping/swapping.h>         // Puede requerir swapping
#include <loggers/logger.h>
```

### `paginacion/tabla_paginas.h`
**Tabla de páginas**

```c
#include <stdint.h>
#include <commons/collections/list.h>

// Estructura de entrada de tabla de páginas
// No incluye otros módulos complejos
```

### `swapping/swapping.h`
**Algoritmo de swapping**

```c
#include <stdint.h>
#include <stdbool.h>

#include <swapping/algoritmo_reemplazo.h>  // Usa algoritmo de reemplazo
#include <paginacion/tabla_paginas.h>      // Modifica tabla de páginas
#include <marcos/marcos.h>                 // Gestiona marcos
#include <conexiones/filesystem.h>         // Para swap in/out con FS
#include <loggers/logger.h>
```

### `swapping/algoritmo_reemplazo.h`
**Algoritmos de reemplazo (LRU, FIFO, Clock)**

```c
#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>  // Para LRU (timestamps)

#include <paginacion/tabla_paginas.h>  // Para acceder a entradas
// No incluye otros módulos complejos
```

### `marcos/marcos.h`
**Gestión de marcos de página**

```c
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/list.h>

// No incluye otros módulos internos complejos (gestión básica)
```

### `procesos/procesos.h`
**Gestión de procesos en memoria**

```c
#include <stdint.h>
#include <commons/collections/list.h>

// Estructura de proceso en memoria
// Puede incluir estructuras compartidas con kernel (PCB simplificado)
// No incluye otros módulos internos directamente
```

### `instrucciones/instrucciones.h`
**Lectura de instrucciones desde memoria**

```c
#include <stdint.h>

#include <gestion/gestion.h>  // Para acceder a memoria del proceso
#include <segmentacion/segmentacion.h>  // O segmentación
#include <paginacion/paginacion.h>      // O paginación
```

### `tlb/tlb.h`
**Translation Lookaside Buffer**

```c
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/list.h>

#include <paginacion/tabla_paginas.h>  // Para fallback si no hay en TLB
// No incluye otros módulos complejos
```

### `loggers/logger.h`
**Wrappers de logging para Memoria**

```c
#include <commons/log.h>

// No incluye otros módulos internos
```

---

## 📁 MÓDULO FILESYSTEM

### `mod_filesystem.h`
**Archivo principal del módulo FileSystem**

```c
#include <stdint.h>
#include <pthread.h>

// Módulos internos
#include <config/fs_config.h>
#include <conexiones/memoria.h>
#include <estructura/superblock.h>
#include <estructura/bitmap.h>
#include <estructura/bloques.h>
#include <archivos/archivos.h>
#include <archivos/metadata.h>
#include <asignacion/asignacion.h>
#include <asignacion/indexado.h>
#include <operaciones/crear.h>
#include <operaciones/leer.h>
#include <operaciones/escribir.h>
#include <operaciones/truncar.h>
#include <hash/hash.h>
#include <directorios/directorios.h>
#include <loggers/logger.h>

// Externos
extern t_log* logger;
extern t_fs_config FS_CONF;
```

### `config/fs_config.h`
**Configuración del módulo FileSystem**

```c
#include <stdint.h>
#include <stdlib.h>

// No incluye otros módulos internos
```

### `conexiones/memoria.h`
**Conexión con módulo Memoria**

```c
#include <stdint.h>
#include <utils/conexiones.h>
#include <loggers/logger.h>

// Estructuras para comunicación con Memoria (swapping, dumps)
```

### `estructura/superblock.h`
**Superbloque del FileSystem**

```c
#include <stdint.h>

// Estructura del superbloque
// No incluye otros módulos internos (metadatos básicos)
```

### `estructura/bitmap.h`
**Gestión del bitmap**

```c
#include <stdint.h>
#include <stdbool.h>

// No incluye otros módulos internos (gestión básica de bits)
```

### `estructura/bloques.h`
**Gestión de bloques**

```c
#include <stdint.h>

// No incluye otros módulos internos (I/O básico de bloques)
```

### `archivos/archivos.h`
**Operaciones sobre archivos**

```c
#include <stdint.h>
#include <stdbool.h>

#include <archivos/metadata.h>        // Necesita metadata
#include <estructura/bloques.h>       // Para leer/escribir bloques
#include <estructura/bitmap.h>        // Para gestionar bloques libres
#include <asignacion/asignacion.h>    // Para asignar bloques
#include <loggers/logger.h>
```

### `archivos/metadata.h`
**Metadata de archivos**

```c
#include <stdint.h>

#include <estructura/superblock.h>  // Para validaciones
#include <hash/hash.h>              // Si usa hash para búsqueda
// Estructura básica de metadata
```

### `asignacion/asignacion.h`
**Algoritmos de asignación de bloques**

```c
#include <stdint.h>
#include <stdbool.h>

#include <asignacion/indexado.h>    // Puede usar asignación indexada
#include <estructura/bitmap.h>      // Para marcar bloques
#include <estructura/bloques.h>     // Para acceder a bloques
#include <loggers/logger.h>
```

### `asignacion/indexado.h`
**Asignación indexada**

```c
#include <stdint.h>

#include <estructura/bloques.h>     // Para leer/escribir bloque índice
#include <estructura/bitmap.h>      // Para gestionar bloques
// No incluye otros módulos complejos
```

### `operaciones/crear.h`
**Creación de archivos**

```c
#include <stdint.h>
#include <stdbool.h>

#include <archivos/metadata.h>      // Para crear metadata
#include <estructura/bitmap.h>      // Para reservar bloques
#include <asignacion/asignacion.h>  // Para asignar bloques
#include <estructura/bloques.h>     // Para escribir bloques
#include <loggers/logger.h>
```

### `operaciones/leer.h`
**Lectura de archivos**

```c
#include <stdint.h>

#include <archivos/metadata.h>      // Para obtener info del archivo
#include <asignacion/indexado.h>    // Para encontrar bloques
#include <estructura/bloques.h>     // Para leer bloques
#include <loggers/logger.h>
```

### `operaciones/escribir.h`
**Escritura de archivos**

```c
#include <stdint.h>

#include <archivos/metadata.h>      // Para actualizar metadata
#include <estructura/bitmap.h>      // Para reservar nuevos bloques si es necesario
#include <asignacion/indexado.h>    // Para encontrar bloques
#include <estructura/bloques.h>     // Para escribir bloques
#include <loggers/logger.h>
```

### `operaciones/truncar.h`
**Truncado de archivos**

```c
#include <stdint.h>

#include <archivos/metadata.h>      // Para actualizar tamaño
#include <estructura/bitmap.h>      // Para liberar bloques
#include <estructura/bloques.h>     // Para limpiar bloques
#include <loggers/logger.h>
```

### `hash/hash.h`
**Función de hash para índices**

```c
#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/list.h>

// Implementación de hash
// No incluye otros módulos internos complejos
```

### `directorios/directorios.h`
**Gestión de estructura de directorios**

```c
#include <stdint.h>
#include <commons/collections/list.h>

#include <archivos/metadata.h>      // Los directorios contienen archivos
#include <hash/hash.h>              // Si usa hash para búsqueda
// No incluye otros módulos complejos
```

### `loggers/logger.h`
**Wrappers de logging para FileSystem**

```c
#include <commons/log.h>

// No incluye otros módulos internos
```

---

## 🔗 DEPENDENCIAS ENTRE MÓDULOS

### Diagrama de Dependencias Principales

```
┌──────────┐         ┌──────────┐         ┌──────────┐
│  Kernel  │────────>│   CPU    │────────>│ Memoria  │
└──────────┘         └──────────┘         └──────────┘
     │                    │                    │
     │                    │                    │
     └────────────────────┴────────────────────┘
                           │
                           │
                      ┌──────────┐
                      │FileSystem│
                      └──────────┘
```

### Includes Trans-Módulo (Headers Compartidos)

#### En `cpu/conexiones/kernel.h`:
```c
// NO incluye mod_kernel.h directamente
// Usa estructuras definidas en include/ipc.h o utils
#include <utils/conexiones.h>  // Para funciones de conexión
```

#### En `cpu/conexiones/memoria.h`:
```c
// NO incluye mod_memoria.h directamente
// Usa protocolo de comunicación definido en include/ipc.h
#include <utils/conexiones.h>
```

#### En `memoria/conexiones/cpu.h`:
```c
// NO incluye mod_cpu.h directamente
#include <utils/conexiones.h>
```

#### En `memoria/conexiones/kernel.h`:
```c
// NO incluye mod_kernel.h directamente
#include <utils/conexiones.h>
```

#### En `memoria/conexiones/filesystem.h`:
```c
// NO incluye mod_filesystem.h directamente
#include <utils/conexiones.h>
```

#### En `filesystem/conexiones/memoria.h`:
```c
// NO incluye mod_memoria.h directamente
#include <utils/conexiones.h>
```

### Archivos Compartidos (include/)

Se recomienda crear en `include/` o `utils/include/`:

#### `ipc.h` (Inter-Process Communication)
**Define estructuras compartidas entre módulos:**

```c
#ifndef IPC_H
#define IPC_H

#include <stdint.h>

// Códigos de operación
typedef enum {
    OP_HANDSHAKE,
    OP_CREATE_PROCESS,
    OP_DISPATCH,           // Kernel -> CPU
    OP_INTERRUPT,          // CPU -> Kernel
    OP_CONTEXT_REPLY,      // CPU -> Kernel
    OP_MEM_INIT,           // Kernel -> Memoria
    OP_MEM_READ,           // CPU -> Memoria
    OP_MEM_WRITE,          // CPU -> Memoria
    OP_MEM_DUMP,           // Memoria -> FileSystem
    OP_FS_READ,            // Memoria -> FileSystem
    OP_FS_WRITE,           // Memoria -> FileSystem
    OP_OK,
    OP_ERROR
} op_code_t;

// Estructuras de contexto (compartidas CPU-Kernel)
typedef struct {
    uint32_t pid;
    uint32_t program_counter;
    // ... registros, etc.
} t_contexto;

#endif
```

---

## 📝 REGLAS DE DEPENDENCIAS

### ✅ Buenas Prácticas:

1. **Headers de Configuración:**
   - Solo incluyen tipos básicos (`stdint.h`, `stdlib.h`)
   - NO incluyen otros módulos internos

2. **Headers de Conexiones:**
   - Incluyen `utils/conexiones.h`
   - Incluyen `loggers/logger.h`
   - NO incluyen headers principales de otros módulos (evitar dependencias circulares)
   - Usan estructuras definidas en `include/ipc.h`

3. **Headers Principales (mod_*.h):**
   - Incluyen la mayoría de headers internos del módulo
   - Definen variables globales (`extern`)
   - Son el punto de entrada del módulo

4. **Headers de Lógica Interna:**
   - Incluyen solo los headers necesarios para su funcionamiento
   - Siguen principio de dependencia mínima
   - Evitan dependencias circulares

5. **Headers de Logging:**
   - Solo incluyen `commons/log.h`
   - NO incluyen otros módulos internos

### ❌ Evitar:

1. **Dependencias Circulares:**
   - `mod_cpu.h` incluye `conexiones/kernel.h`
   - `conexiones/kernel.h` NO debe incluir `mod_cpu.h`

2. **Includes Innecesarios:**
   - No incluir headers completos si solo necesitas un tipo
   - Usar forward declarations cuando sea posible

3. **Headers Trans-Módulo Directos:**
   - `cpu/` NO debe incluir `memoria/mod_memoria.h` directamente
   - Usar estructuras compartidas en `include/ipc.h`

---

## 🎯 EJEMPLOS DE USO

### Ejemplo 1: CPU ejecutando instrucción

```c
// ciclo.c incluye:
#include <ciclo_instruccion/ciclo.h>  // Que incluye:
                                        // - instrucciones/instrucciones.h
                                        // - registros/registros.h
                                        // - mmu/mmu.h
                                        // - interrupciones/interrupciones.h

// ciclo.c puede usar:
void ciclo_ejecutar() {
    instruccion_t* inst = fetch_instruccion();        // De instrucciones
    uint32_t valor = registro_leer(AX);              // De registros
    uint32_t dir_fisica = mmu_traducir(dir_logica);  // De mmu
    if (interrupcion_pendiente()) {                   // De interrupciones
        // ...
    }
}
```

### Ejemplo 2: Memoria gestionando paginación

```c
// gestion.c incluye:
#include <gestion/gestion.h>  // Que incluye:
                               // - paginacion/paginacion.h
                               // - marcos/marcos.h
                               // - procesos/procesos.h

// gestion.c puede usar:
void memoria_asignar(uint32_t pid, uint32_t tamanio) {
    if (MEM_CONF.esquema == PAGINACION) {
        pagina_asignar(pid, tamanio);    // De paginacion
    } else {
        segmento_crear(pid, tamanio);    // De segmentacion
    }
    marco_asignar(pid);                  // De marcos
}
```

### Ejemplo 3: FileSystem creando archivo

```c
// crear.c incluye:
#include <operaciones/crear.h>  // Que incluye:
                                 // - archivos/metadata.h
                                 // - estructura/bitmap.h
                                 // - asignacion/asignacion.h
                                 // - estructura/bloques.h

// crear.c puede usar:
void archivo_crear(char* nombre, uint32_t tamanio) {
    metadata_crear(nombre);           // De archivos/metadata
    bitmap_marcar_ocupado(bloque);    // De estructura/bitmap
    asignacion_indexada_reservar();   // De asignacion/asignacion
    bloque_escribir(bloque, datos);   // De estructura/bloques
}
```

---

**Última actualización:** 2026
**Basado en:** Estructura de Módulos - TP Sistemas Operativos UTN FRBA

