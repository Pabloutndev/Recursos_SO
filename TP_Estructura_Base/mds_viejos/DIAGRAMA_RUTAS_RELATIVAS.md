# Mapeo de Rutas Relativas

## Estructura de Directorios

```
TP_Estructura_Base/
├── kernel/
│   ├── bin/
│   │   └── kernel (ejecutable)
│   └── src/
│       ├── peticiones/
│       │   ├── ruta_procesos.c  ← Define RUTA_BASE_PROCESOS_KERNEL
│       │   └── proceso.c
│       └── ...
│
├── memoria/
│   ├── bin/
│   │   └── memoria (ejecutable)
│   ├── procesos/  ← Archivos de proceso
│   │   ├── process1.txt
│   │   ├── process2.txt
│   │   ├── process3.txt
│   │   └── process4.txt
│   └── src/
│       ├── gestion/
│       │   ├── memoria_core.c  ← Define RUTA_BASE_PROCESOS_MEMORIA
│       │   └── memoria_core.h
│       └── ...
│
└── utils/
    ├── src/
    │   └── common/
    │       ├── shared.c  ← leer_instrucciones()
    │       └── shared.h
    └── ...
```

## Ejecución y Rutas

### Kernel ejecutándose

```
$ cd kernel && ./bin/kernel

Directorio actual:    /path/to/TP_Estructura_Base/kernel/
Ruta a procesos:      ../memoria/procesos/
Archivo validado:     ../memoria/procesos/process1.txt

RUTA_BASE_PROCESOS_KERNEL = "../memoria/procesos/"
```

**Función**: `validar_existe_proceso_kernel("process1.txt")`
```c
#define RUTA_BASE_PROCESOS_KERNEL "../memoria/procesos/"
char ruta[512];
snprintf(ruta, 512, "%s%s", RUTA_BASE_PROCESOS_KERNEL, "process1.txt");
// Resultado: "../memoria/procesos/process1.txt"
```

### Memoria ejecutándose

```
$ cd memoria && ./bin/memoria

Directorio actual:    /path/to/TP_Estructura_Base/memoria/
Ruta a procesos:      procesos/
Archivo leído:        procesos/process1.txt

RUTA_BASE_PROCESOS_MEMORIA = "procesos/"
```

**Función**: `construir_ruta_proceso_memoria("process1.txt")`
```c
#define RUTA_BASE_PROCESOS_MEMORIA "procesos/"
char ruta[512];
snprintf(ruta, 512, "%s%s", RUTA_BASE_PROCESOS_MEMORIA, "process1.txt");
// Resultado: "procesos/process1.txt"
```

## Ejemplo: Ejecución del comando RUN

### 1. Usuario en consola Kernel

```
kernel> RUN process1
```

### 2. Kernel procesa (desde /kernel)

```
ejecutar_proceso("process1")
    ↓
nombre = construir_nombre_proceso("process1")
    → Normaliza a "process1.txt"
    ↓
validar_existe_proceso_kernel("process1.txt")
    → Busca en: ../memoria/procesos/process1.txt ✓ Encontrado
    ↓
Crea PCB(pid=1, path="process1.txt")  ← Envía NOMBRE, no ruta
    ↓
Encola en NEW
```

### 3. Largo Plazo Scheduler envía a Memoria

```
Mensaje: OP_MEM_INIT_PROCESO
{
  pid: 1,
  path: "process1.txt"  ← Solo nombre, no ruta kernel-relativa
}
```

### 4. Memoria recibe y procesa (desde /memoria)

```
memoria_crear_proceso(1, "process1.txt")
    ↓
ruta = construir_ruta_proceso_memoria("process1.txt")
    → Construye: "procesos/process1.txt"
    ↓
leer_instrucciones("procesos/process1.txt")
    → Abre desde /memoria/procesos/process1.txt ✓
    ↓
Lee líneas del archivo
    ↓
Almacena en diccionario
    ↓
Responde OK a Kernel
```

## Cambios si estructura cambia

### Escenario: Procesos se mueven a otra carpeta

**ANTES**:
```
kernel/src/peticiones/ruta_procesos.c:
  #define RUTA_BASE_PROCESOS_KERNEL "../memoria/procesos/"

memoria/src/gestion/memoria_core.c:
  #define RUTA_BASE_PROCESOS_MEMORIA "procesos/"
```

**DESPUÉS** (si procesos en `/TP_Estructura_Base/recursos/procesos`):
```
kernel/src/peticiones/ruta_procesos.c:
  #define RUTA_BASE_PROCESOS_KERNEL "../../recursos/procesos/"

memoria/src/gestion/memoria_core.c:
  #define RUTA_BASE_PROCESOS_MEMORIA "../recursos/procesos/"
```

⚠️ **Nota**: El protocolo `t_mem_init_proceso` sigue conteniendo "process1.txt" 
sin cambios, porque es agnóstico a las rutas relativas de cada módulo.
