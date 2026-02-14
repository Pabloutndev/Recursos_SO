# Guía de Uso: Comando RUN

## Estructura de Procesos

**Ubicación única de archivos de proceso:**
```
memoria/procesos/
├── process1.txt
├── process2.txt
├── process3.txt
└── process4.txt
```

Cada archivo contiene las instrucciones del proceso, una por línea.

## Cómo usar RUN desde la consola

### Opción 1: Escribir solo el nombre (sin extensión)
```
kernel> RUN process1
```

### Opción 2: Escribir el nombre con extensión
```
kernel> RUN process1.txt
```

## Flujo y Rutas Relativas

### **Kernel** (se ejecuta desde `/kernel`)
```
Directorio de ejecución: /kernel
Ruta a procesos:         ../memoria/procesos/
Ejemplo absoluto:        /kernel/../memoria/procesos/process1.txt
```

**Función**: `construir_nombre_proceso()` en `ruta_procesos.c`
- Normaliza nombre: "process1" → "process1.txt"
- Valida existencia en: `../memoria/procesos/process1.txt`
- Envía a Memoria: "process1.txt" (solo el nombre)

### **Memoria** (se ejecuta desde `/memoria`)
```
Directorio de ejecución: /memoria
Ruta a procesos:         procesos/
Ejemplo absoluto:        /memoria/procesos/process1.txt
```

**Función**: `construir_ruta_proceso_memoria()` en `memoria_core.c`
- Recibe de Kernel: "process1.txt"
- Construye ruta: "procesos/process1.txt"
- Lee archivo usando: `leer_instrucciones(ruta)`

## Flujo Interno Completo

```
1. Usuario escribe: 
   RUN process1

2. KERNEL (en /kernel):
   ├─ Normaliza: "process1" → "process1.txt"
   ├─ Valida en: ../memoria/procesos/process1.txt ✓
   ├─ Crea PCB (PID=1, path="process1.txt")
   └─ Encola en NEW

3. Largo Plazo Scheduler (KERNEL):
   ├─ Desencola PCB
   ├─ Envía a Memoria: OP_MEM_INIT_PROCESO
   │  ├─ pid: 1
   │  └─ path: "process1.txt" (nombre solamente)

4. MEMORIA (en /memoria):
   ├─ Recibe: path="process1.txt"
   ├─ Construye ruta local: procesos/process1.txt
   ├─ Abre archivo
   ├─ Lee instrucciones con leer_instrucciones()
   ├─ Almacena en diccionario
   └─ Responde OK a Kernel

5. KERNEL recibe OK:
   ├─ Mueve PCB a READY
   └─ Continúa planificación

6. Corto Plazo Scheduler (KERNEL):
   └─ Ejecuta proceso
```

## Procesos de ejemplo disponibles

- `process1.txt` - Proceso ejemplo 1
- `process2.txt` - Proceso ejemplo 2  
- `process3.txt` - Proceso ejemplo 3
- `process4.txt` - Proceso ejemplo 4

## Formato de archivo de proceso

Cada archivo contiene instrucciones, una por línea:

**Ejemplo: process1.txt**
```
SET A 5
SET B 10
SUM
MOV_OUT
```

Líneas vacías se ignoran automáticamente.

## Ventajas de esta arquitectura

✅ **Cada módulo conoce su propia estructura**
- KERNEL busca en: `../memoria/procesos/` (desde su directorio)
- MEMORIA busca en: `procesos/` (desde su directorio)
- Rutas relativas correctas para cada entorno

✅ **El nombre del proceso es agnóstico**
- KERNEL valida localmente
- MEMORIA recibe solo el nombre
- MEMORIA construye su propia ruta

✅ **Portabilidad**
- Si se cambia estructura de directorios, solo se actualizan los defines
- Sin hardcodear rutas en protocolos de comunicación


