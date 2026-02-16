# Video 5 - Creación y Destrucción de Procesos

**Duración estimada:** 10-15 minutos  
**Bloque:** Gestión de Procesos

---

## Conceptos a Explicar

### Ciclo de vida completo
- Nacimiento: ¿de dónde viene un proceso?
- Vida: ejecución de instrucciones
- Muerte: finalización normal o forzada

### Creación en sistemas reales
- `fork()` en Unix/Linux: clonación de proceso padre
- `exec()`: reemplazo de imagen del proceso
- En nuestro TP: creación desde archivo de instrucciones

### Terminación
- **Normal:** instrucción EXIT
- **Forzada:** comando KILL del usuario
- **Error:** segmentation fault, división por cero

---

## Código y Demostración

### 1. Comando RUN: inicio del flujo
**Archivo:** `kernel/src/conexiones/consola_handler.c`

```c
void handle_run_proceso(int socket_consola) {
    t_paquete* paq = recibir_paquete(socket_consola);
    char* path = leer_string(paq);
    
    log_info(logger, "Recibido RUN: %s", path);
    
    if (ejecutar_proceso(path)) {
        enviar_respuesta_ok(socket_consola);
    } else {
        enviar_respuesta_error(socket_consola, "Path inválido");
    }
    
    destruir_paquete(paq);
}
```

### 2. Validación del archivo
**Archivo:** `kernel/src/peticiones/ruta_procesos.c`

```c
bool validar_archivo_proceso(char* path) {
    // Construir ruta completa
    char* ruta_completa = string_from_format("%s/%s", 
        config.PATH_PROCESOS, 
        path
    );
    
    // Verificar existencia
    FILE* archivo = fopen(ruta_completa, "r");
    if (!archivo) {
        log_error(logger, "Archivo no encontrado: %s", ruta_completa);
        free(ruta_completa);
        return false;
    }
    
    fclose(archivo);
    free(ruta_completa);
    return true;
}
```

### 3. Creación del proceso
**Archivo:** `kernel/src/peticiones/proceso.c`

```c
bool ejecutar_proceso(char* path) {
    // 1. Validar archivo
    if (!validar_archivo_proceso(path)) {
        return false;
    }
    
    // 2. Crear PCB
    t_pcb* pcb = pcb_crear(path);
    log_info(logger, "Proceso creado: PID=%d", pcb->pid);
    
    // 3. Encolar en NEW
    pthread_mutex_lock(&mutex_cola_new);
    queue_push(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);
    
    // 4. Notificar al planificador largo plazo
    sem_post(&sem_hay_new);
    
    log_info(logger, "Proceso PID=%d encolado en NEW", pcb->pid);
    return true;
}
```

### 4. Creación en Memoria
**Archivo:** `memoria/src/gestion/memoria_core.c`

```c
bool memoria_crear_proceso(uint32_t pid, char* path) {
    log_info(logger, "Creando proceso PID=%d: %s", pid, path);
    
    // 1. Abrir archivo de instrucciones
    FILE* archivo = fopen(path, "r");
    if (!archivo) {
        log_error(logger, "No se pudo abrir: %s", path);
        return false;
    }
    
    // 2. Leer todas las instrucciones
    t_list* instrucciones = list_create();
    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) {
        // Eliminar salto de línea
        linea[strcspn(linea, "\n")] = 0;
        list_add(instrucciones, string_duplicate(linea));
    }
    fclose(archivo);
    
    // 3. Crear estructura de memoria
    t_proceso_memoria* proceso = malloc(sizeof(t_proceso_memoria));
    proceso->pid = pid;
    proceso->instrucciones = instrucciones;
    proceso->tabla_paginas = crear_tabla_paginas(pid);
    
    // 4. Guardar en diccionario
    dictionary_put(procesos, string_itoa(pid), proceso);
    
    log_info(logger, "Proceso PID=%d creado con %d instrucciones", 
        pid, list_size(instrucciones));
    
    return true;
}
```

---

## Destrucción de Procesos

### 1. Instrucción EXIT (finalización normal)
**Archivo:** `cpu/src/instrucciones/operaciones.c`

```c
t_resultado exit_proceso(t_contexto* ctx) {
    log_info(logger, "Ejecutando EXIT para PID=%d", ctx->pid);
    
    t_resultado res;
    res.motivo = MOTIVO_EXIT;
    res.valor = 0;
    return res;
}
```

### 2. Comando KILL (finalización forzada)
**Archivo:** `kernel/src/planificacion/planificacion.c`

```c
void planificacion_finalizar_proceso(uint32_t pid) {
    log_info(logger, "Finalizando proceso PID=%d (KILL)", pid);
    
    // Buscar el proceso en todas las colas
    t_pcb* pcb = buscar_pcb_en_todas_las_colas(pid);
    if (!pcb) {
        log_warning(logger, "Proceso PID=%d no encontrado", pid);
        return;
    }
    
    // Cambiar estado a EXIT
    pthread_mutex_lock(&pcb->mutex);
    pcb->estado = ESTADO_EXIT;
    pthread_mutex_unlock(&pcb->mutex);
    
    // Liberar recursos
    liberar_recursos_proceso(pcb);
    
    // Finalizar en Memoria
    solicitar_finalizacion_memoria(pcb->pid);
    
    // Finalizar en CPU (si está ejecutando)
    if (proceso_en_cpu == pcb->pid) {
        enviar_interrupcion_cpu(INTERRUPCION_KILL);
    }
    
    // Destruir PCB
    pcb_destruir(pcb);
    
    // Incrementar grado multiprogramación
    sem_post(&sem_mp);
}
```

### 3. Liberación de recursos
```c
void liberar_recursos_proceso(t_pcb* pcb) {
    log_info(logger, "Liberando recursos de PID=%d", pcb->pid);
    
    // Liberar todos los recursos adquiridos
    LIST_ITERATE(pcb->recursos_adquiridos, char*, nombre_recurso) {
        t_recurso* recurso = dictionary_get(recursos, nombre_recurso);
        recurso_signal_interno(recurso);
        
        log_info(logger, "Liberado recurso %s de PID=%d", 
            nombre_recurso, pcb->pid);
    }
}
```

### 4. Destrucción en Memoria
**Archivo:** `memoria/src/gestion/memoria_core.c`

```c
void memoria_finalizar_proceso(uint32_t pid) {
    log_info(logger, "Finalizando proceso PID=%d en Memoria", pid);
    
    char* key = string_itoa(pid);
    t_proceso_memoria* proceso = dictionary_remove(procesos, key);
    free(key);
    
    if (!proceso) {
        log_warning(logger, "Proceso PID=%d no existe en Memoria", pid);
        return;
    }
    
    // Liberar instrucciones
    list_destroy_and_destroy_elements(proceso->instrucciones, free);
    
    // Liberar frames (según esquema: paginación o segmentación)
    destruir_tabla_paginas(proceso->tabla_paginas);
    
    free(proceso);
    log_info(logger, "Proceso PID=%d finalizado en Memoria", pid);
}
```

---

## Demo: Test 1 (Ciclo Básico)

### Archivo de instrucciones: `test1.txt`
```
SET AX 10
SET BX 20
SUM AX BX
EXIT
```

### Ejecución paso a paso
```bash
# Terminal 1: Memoria
$ ./bin/memoria memoria.config
[MEMORIA] Servidor iniciado en puerto 8003

# Terminal 2: CPU
$ ./bin/cpu cpu.config
[CPU] Conectado a Memoria
[CPU] Servidor iniciado en puerto 8002

# Terminal 3: Kernel
$ ./bin/kernel kernel.config
[KERNEL] Conectado a Memoria
[KERNEL] Conectado a CPU
[KERNEL] Servidor iniciado en puerto 8001

# Terminal 4: Consola
$ ./bin/consola consola.config
so> RUN test1.txt
[OK] Proceso creado con PID: 1

so> START
[OK] Planificación iniciada
```

### Logs esperados
```
[KERNEL] Recibido RUN: test1.txt
[KERNEL] Proceso creado: PID=1
[KERNEL] Proceso PID=1 encolado en NEW
[KERNEL] Planificador LP: Admitiendo PID=1
[MEMORIA] Creando proceso PID=1: test1.txt
[MEMORIA] Proceso PID=1 creado con 4 instrucciones
[KERNEL] Transición: PID=1, NEW → READY
[KERNEL] Planificador CP: Despachando PID=1
[KERNEL] Transición: PID=1, READY → EXEC
[CPU] Recibido contexto PID=1
[CPU] Fetch: SET AX 10
[CPU] Ejecutando SET: AX=10
[CPU] Fetch: SET BX 20
[CPU] Ejecutando SET: BX=20
[CPU] Fetch: SUM AX BX
[CPU] Ejecutando SUM: AX=30
[CPU] Fetch: EXIT
[CPU] Ejecutando EXIT
[KERNEL] Proceso PID=1 finalizó (EXIT)
[KERNEL] Transición: PID=1, EXEC → EXIT
[MEMORIA] Finalizando proceso PID=1 en Memoria
[KERNEL] Destruyendo PCB: PID=1
```

---

## Puntos Clave a Destacar

1. **Cooperación:** Crear/destruir proceso requiere coordinación Kernel-Memoria
2. **Atomicidad:** La creación es todo-o-nada (rollback si falla la memoria)
3. **Limpieza:** Es crítico liberar TODOS los recursos (memoria, archivos, semáforos)
4. **Trazabilidad:** Logs detallados permiten debugging

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos el **Planificador de Largo Plazo**
- Grado de multiprogramación
- Cómo se controla la admisión de procesos
- Semáforos para coordinación
