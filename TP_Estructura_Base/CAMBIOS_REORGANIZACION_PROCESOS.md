# Cambios: Reorganización de Procesos con Rutas Relativas Correctas

## ✅ Cambios realizados

### 1. Estructura única de procesos
- **ANTES**: `memoria/instrucciones/` (carpeta para archivos)
- **AHORA**: `memoria/procesos/` (carpeta única con archivos de proceso)
- Eliminada carpeta `instrucciones`

### 2. Función compartida para leer instrucciones
**Archivo**: `utils/src/common/shared.c` y `shared.h`

Añadidas funciones:
- `char** leer_instrucciones(const char* path, uint32_t* cantidad)`
- `void liberar_instrucciones(char** instrucciones, uint32_t cantidad)`

### 3. **NUEVO**: Rutas relativas correctas por entorno

**KERNEL** se ejecuta desde `/kernel`:
- Ruta a procesos: `../memoria/procesos/`
- Función: `validar_existe_proceso_kernel()` valida archivo existe localmente

**MEMORIA** se ejecuta desde `/memoria`:
- Ruta a procesos: `procesos/`
- Función: `construir_ruta_proceso_memoria()` construye ruta local

### 4. Cambio de protocolo (semántica)

**ANTES**: Campo `path` en `t_mem_init_proceso` contenía ruta kernel-relativa
```
{
  pid: 1,
  path: "../memoria/procesos/process1.txt"  ❌ No funciona desde Memoria
}
```

**AHORA**: Campo `path` en `t_mem_init_proceso` contiene solo el NOMBRE
```
{
  pid: 1,
  path: "process1.txt"  ✓ Memoria construye su propia ruta
}
```

### 5. Actualización de funciones

**kernel/src/peticiones/ruta_procesos.c**
- `construir_nombre_proceso()` - Normaliza nombre (agrega .txt si falta)
- `validar_existe_proceso_kernel()` - Valida usando ruta kernel-relativa

**memoria/src/gestion/memoria_core.c**
- `construir_ruta_proceso_memoria()` - Construye ruta memoria-relativa
- `memoria_crear_proceso()` - Usa ruta local para leer instrucciones

### 6. Flujo actualizado

```
Usuario: RUN process1
    ↓
KERNEL (en /kernel):
  - Normaliza: "process1" → "process1.txt"
  - Valida en: ../memoria/procesos/process1.txt ✓
  - Crea PCB(pid=1, path="process1.txt")
  - Encola en NEW
    ↓
Largo Plazo Scheduler (KERNEL):
  - Envía a Memoria: OP_MEM_INIT_PROCESO {pid=1, path="process1.txt"}
    ↓
MEMORIA (en /memoria):
  - Recibe: path="process1.txt" (nombre solamente)
  - Construye ruta: procesos/process1.txt
  - Lee instrucciones
  - Almacena y responde OK
    ↓
KERNEL:
  - Mueve PCB a READY
  - Continúa planificación
```

## Ventajas

✅ **Arquitectura robusta**
- Cada módulo usa rutas relativas correctas para su entorno
- Sin hardcoding de rutas absolutas
- Fácil de reconfigurar si cambia estructura

✅ **Protocolo agnóstico**
- El mensaje entre módulos solo contiene el nombre
- No depende de rutas relativas del emisor

✅ **Validación en origen**
- KERNEL valida que el archivo existe ANTES de encolar
- Evita operaciones innecesarias en Memoria

✅ **Portabilidad**
- Código funciona desde diferentes directorios
- Sin dependencias de Working Directory específico

