# Estructura de Módulos - TP Sistemas Operativos UTN FRBA

## 📋 Índice
1. [Módulo CPU](#módulo-cpu)
2. [Módulo Memoria](#módulo-memoria)
3. [Módulo FileSystem](#módulo-filesystem)
4. [Justificación de Archivos](#justificación-de-archivos)

---

## 🖥️ Módulo CPU

### Estructura de Directorios

```
cpu/
├── src/
│   ├── main.c                    # Punto de entrada, inicialización
│   ├── mod_cpu.c                 # Módulo principal CPU
│   ├── mod_cpu.h                 # Header del módulo
│   │
│   ├── config/
│   │   ├── cpu_config.c          # Carga de configuración
│   │   └── cpu_config.h          # Estructura de configuración
│   │
│   ├── conexiones/
│   │   ├── kernel.c              # Conexión con Kernel (dispatch/interrupt)
│   │   ├── kernel.h
│   │   ├── memoria.c              # Conexión con Memoria
│   │   └── memoria.h
│   │
│   ├── ciclo_instruccion/
│   │   ├── ciclo.c                # Ciclo fetch-decode-execute
│   │   └── ciclo.h
│   │
│   ├── instrucciones/
│   │   ├── instrucciones.c       # Decodificación y ejecución
│   │   ├── instrucciones.h
│   │   ├── operaciones.c          # Operaciones aritméticas/lógicas
│   │   └── operaciones.h
│   │
│   ├── registros/
│   │   ├── registros.c            # Gestión de registros (AX, BX, CX, DX, etc.)
│   │   └── registros.h
│   │
│   ├── mmu/
│   │   ├── mmu.c                  # Memory Management Unit (traducción de direcciones)
│   │   └── mmu.h
│   │
│   ├── interrupciones/
│   │   ├── interrupciones.c       # Manejo de interrupciones
│   │   └── interrupciones.h
│   │
│   ├── contexto/
│   │   ├── contexto.c             # Serialización/deserialización de contexto
│   │   └── contexto.h
│   │
│   └── loggers/
│       ├── logger.c               # Wrappers de logging específicos
│       └── logger.h
│
├── cpu.config                     # Archivo de configuración
├── makefile
└── settings.mk
```

---

## 💾 Módulo Memoria

### Estructura de Directorios

```
memoria/
├── src/
│   ├── main.c                     # Punto de entrada, inicialización
│   ├── mod_memoria.c              # Módulo principal Memoria
│   ├── mod_memoria.h
│   │
│   ├── config/
│   │   ├── memoria_config.c      # Carga de configuración
│   │   └── memoria_config.h
│   │
│   ├── conexiones/
│   │   ├── cpu.c                  # Conexión con CPU
│   │   ├── cpu.h
│   │   ├── kernel.c               # Conexión con Kernel
│   │   ├── kernel.h
│   │   ├── filesystem.c           # Conexión con FileSystem
│   │   └── filesystem.h
│   │
│   ├── gestion/
│   │   ├── gestion.c              # Gestión de memoria (segmentación/paginación)
│   │   ├── gestion.h
│   │   ├── asignacion.c            # Algoritmos de asignación
│   │   └── asignacion.h
│   │
│   ├── segmentacion/
│   │   ├── segmentacion.c         # Implementación de segmentación
│   │   ├── segmentacion.h
│   │   ├── tabla_segmentos.c      # Tabla de segmentos por proceso
│   │   └── tabla_segmentos.h
│   │
│   ├── paginacion/
│   │   ├── paginacion.c           # Implementación de paginación
│   │   ├── paginacion.h
│   │   ├── tabla_paginas.c        # Tabla de páginas por proceso
│   │   └── tabla_paginas.h
│   │
│   ├── swapping/
│   │   ├── swapping.c             # Algoritmo de swapping
│   │   ├── swapping.h
│   │   ├── algoritmo_reemplazo.c  # LRU, FIFO, Clock, etc.
│   │   └── algoritmo_reemplazo.h
│   │
│   ├── marcos/
│   │   ├── marcos.c               # Gestión de marcos de página
│   │   └── marcos.h
│   │
│   ├── procesos/
│   │   ├── procesos.c              # Gestión de procesos en memoria
│   │   └── procesos.h
│   │
│   ├── instrucciones/
│   │   ├── instrucciones.c         # Lectura de instrucciones desde memoria
│   │   └── instrucciones.h
│   │
│   ├── tlb/
│   │   ├── tlb.c                  # Translation Lookaside Buffer
│   │   └── tlb.h
│   │
│   └── loggers/
│       ├── logger.c
│       └── logger.h
│
├── memoria.config
├── makefile
└── settings.mk
```

---

## 📁 Módulo FileSystem

### Estructura de Directorios

```
filesystem/
├── src/
│   ├── main.c                     # Punto de entrada, inicialización
│   ├── mod_filesystem.c           # Módulo principal FileSystem
│   ├── mod_filesystem.h
│   │
│   ├── config/
│   │   ├── fs_config.c            # Carga de configuración
│   │   └── fs_config.h
│   │
│   ├── conexiones/
│   │   ├── memoria.c               # Conexión con Memoria
│   │   └── memoria.h
│   │
│   ├── estructura/
│   │   ├── superblock.c            # Superbloque del FS
│   │   ├── superblock.h
│   │   ├── bitmap.c                # Gestión del bitmap
│   │   ├── bitmap.h
│   │   ├── bloques.c               # Gestión de bloques
│   │   └── bloques.h
│   │
│   ├── archivos/
│   │   ├── archivos.c              # Operaciones sobre archivos
│   │   ├── archivos.h
│   │   ├── metadata.c              # Metadata de archivos
│   │   └── metadata.h
│   │
│   ├── asignacion/
│   │   ├── asignacion.c            # Algoritmos de asignación de bloques
│   │   ├── asignacion.h
│   │   ├── indexado.c              # Asignación indexada
│   │   └── indexado.h
│   │
│   ├── operaciones/
│   │   ├── crear.c                 # Crear archivo
│   │   ├── crear.h
│   │   ├── leer.c                  # Leer archivo
│   │   ├── leer.h
│   │   ├── escribir.c              # Escribir archivo
│   │   ├── escribir.h
│   │   ├── truncar.c               # Truncar archivo
│   │   └── truncar.h
│   │
│   ├── hash/
│   │   ├── hash.c                  # Función de hash para índices
│   │   └── hash.h
│   │
│   ├── directorios/
│   │   ├── directorios.c           # Gestión de estructura de directorios
│   │   └── directorios.h
│   │
│   └── loggers/
│       ├── logger.c
│       └── logger.h
│
├── filesystem.config
├── makefile
└── settings.mk
```

---

## 📚 Justificación de Archivos

### 🖥️ MÓDULO CPU

#### `ciclo_instruccion/ciclo.c`
**Concepto Teórico:** Ciclo de Instrucción (Fetch-Decode-Execute)
- **Justificación:** Implementa el ciclo fundamental de ejecución de instrucciones. Fetch obtiene la instrucción de memoria, Decode la interpreta, Execute la ejecuta.
- **Funciones típicas:**
  - `ciclo_ejecutar()` - Loop principal del ciclo
  - `fetch_instruccion()` - Obtener instrucción de memoria
  - `decode_instruccion()` - Decodificar opcode
  - `execute_instruccion()` - Ejecutar operación

#### `instrucciones/instrucciones.c`
**Concepto Teórico:** Set de Instrucciones, Decodificación
- **Justificación:** Contiene la lógica de decodificación de opcodes y el dispatch a operaciones específicas. Similar al concepto de "instruction decoder" en arquitectura de computadoras.
- **Funciones típicas:**
  - `decodificar_instruccion()` - Identificar tipo de instrucción
  - `ejecutar_instruccion()` - Dispatch a operación específica
  - `validar_instruccion()` - Verificar sintaxis

#### `instrucciones/operaciones.c`
**Concepto Teórico:** ALU (Arithmetic Logic Unit)
- **Justificación:** Implementa las operaciones aritméticas y lógicas que puede realizar el CPU. Representa la unidad de procesamiento.
- **Funciones típicas:**
  - `sumar()`, `restar()`, `multiplicar()`, `dividir()`
  - `AND()`, `OR()`, `XOR()`, `NOT()`
  - `comparar()`, `desplazar()`

#### `registros/registros.c`
**Concepto Teórico:** Registros del Procesador
- **Justificación:** Gestiona los registros de propósito general (AX, BX, CX, DX) y especiales (PC, SP). Representa el estado interno del procesador.
- **Funciones típicas:**
  - `registro_leer()` - Leer valor de registro
  - `registro_escribir()` - Escribir valor en registro
  - `registros_guardar()` - Guardar contexto completo
  - `registros_restaurar()` - Restaurar contexto

#### `mmu/mmu.c`
**Concepto Teórico:** Memory Management Unit, Traducción de Direcciones
- **Justificación:** Traduce direcciones lógicas a físicas. Implementa el concepto de memoria virtual y protección de memoria.
- **Funciones típicas:**
  - `mmu_traducir()` - Traducir dirección lógica a física
  - `mmu_validar_acceso()` - Verificar permisos
  - `mmu_page_fault()` - Manejar page fault

#### `interrupciones/interrupciones.c`
**Concepto Teórico:** Sistema de Interrupciones
- **Justificación:** Maneja las interrupciones del sistema (quantum, I/O, excepciones). Implementa el mecanismo de cambio de contexto.
- **Funciones típicas:**
  - `interrupcion_recibir()` - Recibir interrupción del kernel
  - `interrupcion_procesar()` - Procesar interrupción
  - `interrupcion_enviar()` - Enviar interrupción al kernel

#### `contexto/contexto.c`
**Concepto Teórico:** Context Switch, Estado del Procesador
- **Justificación:** Serializa y deserializa el contexto de ejecución para permitir el cambio de procesos. Esencial para multiprogramación.
- **Funciones típicas:**
  - `contexto_serializar()` - Convertir contexto a bytes
  - `contexto_deserializar()` - Reconstruir contexto desde bytes
  - `contexto_guardar()` - Guardar contexto actual
  - `contexto_cargar()` - Cargar nuevo contexto

---

### 💾 MÓDULO MEMORIA

#### `gestion/gestion.c`
**Concepto Teórico:** Gestión de Memoria Principal
- **Justificación:** Coordina la gestión general de memoria, decide entre segmentación y paginación según configuración. Punto central de decisiones de asignación.
- **Funciones típicas:**
  - `memoria_asignar()` - Asignar memoria a proceso
  - `memoria_liberar()` - Liberar memoria de proceso
  - `memoria_compactar()` - Compactación (si aplica)

#### `segmentacion/segmentacion.c`
**Concepto Teórico:** Segmentación de Memoria
- **Justificación:** Implementa el esquema de segmentación donde la memoria se divide en segmentos lógicos (código, datos, stack). Cada segmento tiene base y límite.
- **Funciones típicas:**
  - `segmento_crear()` - Crear nuevo segmento
  - `segmento_eliminar()` - Eliminar segmento
  - `segmento_validar_acceso()` - Verificar límites
  - `segmento_traducir()` - Traducir dirección lógica

#### `segmentacion/tabla_segmentos.c`
**Concepto Teórico:** Tabla de Segmentos
- **Justificación:** Mantiene la tabla de segmentos por proceso. Cada entrada contiene base, límite y permisos. Esencial para protección y traducción.
- **Funciones típicas:**
  - `tabla_crear()` - Crear tabla para proceso
  - `tabla_agregar_segmento()` - Agregar entrada
  - `tabla_buscar_segmento()` - Buscar por ID
  - `tabla_destruir()` - Liberar tabla

#### `paginacion/paginacion.c`
**Concepto Teórico:** Paginación
- **Justificación:** Implementa paginación donde la memoria se divide en páginas de tamaño fijo. Permite memoria virtual y swapping eficiente.
- **Funciones típicas:**
  - `pagina_asignar()` - Asignar página
  - `pagina_liberar()` - Liberar página
  - `pagina_traducir()` - Traducir dirección virtual

#### `paginacion/tabla_paginas.c`
**Concepto Teórico:** Tabla de Páginas, Page Table
- **Justificación:** Mantiene la tabla de páginas por proceso. Mapea páginas virtuales a marcos físicos. Puede ser multinivel (2-3 niveles).
- **Funciones típicas:**
  - `tabla_crear()` - Crear tabla de páginas
  - `tabla_agregar_entrada()` - Agregar mapeo página->marco
  - `tabla_buscar_marco()` - Obtener marco físico
  - `tabla_actualizar_bits()` - Actualizar bits de control

#### `swapping/swapping.c`
**Concepto Teórico:** Swapping, Memoria Virtual
- **Justificación:** Implementa el mecanismo de swapping de páginas entre memoria principal y secundaria. Permite ejecutar procesos más grandes que la RAM.
- **Funciones típicas:**
  - `swapping_swap_out()` - Mover página a disco
  - `swapping_swap_in()` - Traer página de disco
  - `swapping_es_necesario()` - Decidir si hacer swap

#### `swapping/algoritmo_reemplazo.c`
**Concepto Teórico:** Algoritmos de Reemplazo de Páginas
- **Justificación:** Implementa algoritmos como LRU, FIFO, Clock para decidir qué página reemplazar cuando no hay marcos libres.
- **Funciones típicas:**
  - `lru_seleccionar_victima()` - LRU
  - `fifo_seleccionar_victima()` - FIFO
  - `clock_seleccionar_victima()` - Clock Algorithm

#### `marcos/marcos.c`
**Concepto Teórico:** Marcos de Página, Frame Management
- **Justificación:** Gestiona los marcos físicos de memoria. Mantiene lista de marcos libres y ocupados. Esencial para asignación eficiente.
- **Funciones típicas:**
  - `marco_asignar()` - Asignar marco libre
  - `marco_liberar()` - Marcar marco como libre
  - `marco_obtener_libre()` - Obtener próximo marco disponible

#### `tlb/tlb.c`
**Concepto Teórico:** Translation Lookaside Buffer
- **Justificación:** Implementa caché de traducciones de direcciones para acelerar el acceso. Reduce accesos a tabla de páginas.
- **Funciones típicas:**
  - `tlb_buscar()` - Buscar traducción en TLB
  - `tlb_agregar()` - Agregar entrada a TLB
  - `tlb_invalidar()` - Invalidar entrada (context switch)

---

### 📁 MÓDULO FILESYSTEM

#### `estructura/superblock.c`
**Concepto Teórico:** Superbloque, Metadatos del Sistema de Archivos
- **Justificación:** Contiene información crítica del FS (tamaño, cantidad de bloques, ubicación de estructuras). Es el "header" del sistema de archivos.
- **Funciones típicas:**
  - `superblock_leer()` - Leer superbloque de disco
  - `superblock_escribir()` - Escribir superbloque
  - `superblock_validar()` - Validar integridad

#### `estructura/bitmap.c`
**Concepto Teórico:** Bitmap de Asignación
- **Justificación:** Representa qué bloques están libres (0) u ocupados (1). Permite asignación rápida de bloques libres.
- **Funciones típicas:**
  - `bitmap_marcar_ocupado()` - Marcar bloque como usado
  - `bitmap_marcar_libre()` - Marcar bloque como libre
  - `bitmap_buscar_libre()` - Encontrar próximo bloque libre
  - `bitmap_contar_libres()` - Contar bloques disponibles

#### `estructura/bloques.c`
**Concepto Teórico:** Gestión de Bloques
- **Justificación:** Gestiona la lectura/escritura de bloques físicos. Abstrae el acceso a bajo nivel al almacenamiento.
- **Funciones típicas:**
  - `bloque_leer()` - Leer bloque del disco
  - `bloque_escribir()` - Escribir bloque al disco
  - `bloque_validar()` - Validar número de bloque

#### `asignacion/asignacion.c`
**Concepto Teórico:** Algoritmos de Asignación de Bloques
- **Justificación:** Implementa estrategias de asignación (contigua, enlazada, indexada). Decide cómo distribuir los bloques de un archivo.
- **Funciones típicas:**
  - `asignacion_contigua()` - Asignación contigua
  - `asignacion_enlazada()` - Asignación enlazada
  - `asignacion_indexada()` - Asignación indexada

#### `asignacion/indexado.c`
**Concepto Teórico:** Asignación Indexada
- **Justificación:** Implementa asignación indexada pura donde un bloque índice contiene punteros a bloques de datos. Permite acceso aleatorio eficiente.
- **Funciones típicas:**
  - `indexado_crear_indice()` - Crear bloque índice
  - `indexado_agregar_bloque()` - Agregar puntero a bloque de datos
  - `indexado_obtener_bloque()` - Obtener bloque por offset

#### `archivos/metadata.c`
**Concepto Teórico:** Metadata de Archivos, Inodos
- **Justificación:** Gestiona la información de archivos (nombre, tamaño, ubicación de bloques, permisos). Similar al concepto de inodo en sistemas Unix.
- **Funciones típicas:**
  - `metadata_crear()` - Crear metadata para nuevo archivo
  - `metadata_leer()` - Leer metadata de archivo
  - `metadata_actualizar()` - Actualizar información
  - `metadata_eliminar()` - Eliminar metadata

#### `operaciones/crear.c`
**Concepto Teórico:** Creación de Archivos
- **Justificación:** Implementa la operación de creación de archivos. Reserva bloques, crea metadata, inicializa estructuras.
- **Funciones típicas:**
  - `archivo_crear()` - Crear nuevo archivo
  - `archivo_validar_nombre()` - Validar nombre único
  - `archivo_reservar_bloques()` - Reservar bloques necesarios

#### `operaciones/leer.c` y `operaciones/escribir.c`
**Concepto Teórico:** Operaciones de I/O
- **Justificación:** Implementan lectura y escritura de archivos. Manejan traducción de offset lógico a bloques físicos, buffering.
- **Funciones típicas:**
  - `archivo_leer()` - Leer datos del archivo
  - `archivo_escribir()` - Escribir datos al archivo
  - `archivo_calcular_bloques()` - Determinar bloques a acceder

#### `hash/hash.c`
**Concepto Teórico:** Función de Hash
- **Justificación:** Implementa función de hash para índices de archivos. Permite acceso rápido a metadata mediante hash table.
- **Funciones típicas:**
  - `hash_calcular()` - Calcular hash de nombre
  - `hash_buscar()` - Buscar en tabla hash
  - `hash_insertar()` - Insertar en tabla hash

---

## 🔗 Relaciones entre Módulos

### Flujo Típico de Ejecución:

1. **Kernel → CPU:** Envía contexto (dispatch)
2. **CPU → Memoria:** Solicita instrucción (fetch)
3. **Memoria → CPU:** Devuelve instrucción
4. **CPU:** Decodifica y ejecuta
5. **CPU → Memoria:** Accede a datos si es necesario
6. **CPU → Kernel:** Envía interrupción (quantum, I/O)
7. **Memoria → FileSystem:** Swapping, dump de memoria
8. **FileSystem → Memoria:** Confirmación de operaciones

---

## 📝 Notas Importantes

1. **Modularidad:** Cada archivo tiene una responsabilidad clara siguiendo el principio de responsabilidad única.

2. **Extensibilidad:** La estructura permite agregar nuevos algoritmos sin modificar código existente (ej: nuevo algoritmo de reemplazo).

3. **Testabilidad:** Cada módulo puede ser testeado independientemente.

4. **Reutilización:** Funciones comunes (conexiones, logging) están centralizadas.

5. **Separación de Concerns:** Lógica de negocio separada de comunicación, configuración y logging.

---

## 🎯 Conceptos Teóricos Cubiertos

### CPU:
- ✅ Ciclo de Instrucción
- ✅ Registros del Procesador
- ✅ Set de Instrucciones
- ✅ MMU y Traducción de Direcciones
- ✅ Sistema de Interrupciones
- ✅ Context Switch

### Memoria:
- ✅ Gestión de Memoria Principal
- ✅ Segmentación
- ✅ Paginación
- ✅ Memoria Virtual
- ✅ Swapping
- ✅ Algoritmos de Reemplazo (LRU, FIFO, Clock)
- ✅ TLB (Translation Lookaside Buffer)
- ✅ Page Faults

### FileSystem:
- ✅ Estructura de Sistemas de Archivos
- ✅ Superbloque
- ✅ Bitmap de Asignación
- ✅ Asignación de Bloques (Contigua, Enlazada, Indexada)
- ✅ Metadata de Archivos
- ✅ Operaciones de I/O
- ✅ Hash Tables para Índices

---

**Última actualización:** 2026
**Basado en:** TPs típicos de Sistemas Operativos UTN FRBA

