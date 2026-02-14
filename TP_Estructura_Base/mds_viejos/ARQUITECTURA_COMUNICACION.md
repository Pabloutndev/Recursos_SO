# Arquitectura de Comunicación entre Módulos

## 📋 Principios Fundamentales

1. **Adaptadores** contienen la lógica para formar y transformar estructuras compartidas
2. **Servidores** SOLO reciben paquetes y delegan a adaptadores
3. **Protocolo/mensajes** maneja serialización/deserialización
4. **Op_codes** definen el tipo de operación y son la "interfaz" entre módulos
5. **Responsabilidad única**: Cada layer hace UNA cosa bien

## 🔄 Flujo de Comunicación Estándar

### Envío (Módulo A → Módulo B)

```
1. LÓGICA INTERNA (A)
   └─ Necesita enviar algo a B
   
2. ADAPTADOR (A)
   ├─ Prepara estructura compartida (ej: t_mem_init_proceso)
   └─ Llama a protocolo/mensajes
   
3. PROTOCOLO (utils/protocolo/mensajes)
   ├─ Serializa estructura + op_code
   └─ Envía paquete por socket
   
4. RED
   └─ Paquete viaja a B
```

### Recepción (Módulo B recibe de A)

```
1. SERVIDOR (B)
   ├─ Recibe paquete
   ├─ Lee op_code
   └─ Delega a adaptador
   
2. ADAPTADOR (B)
   ├─ Deserializa con protocolo/mensajes
   ├─ Convierte a estructura interna de B
   └─ Llama a lógica interna
   
3. LÓGICA INTERNA (B)
   ├─ Procesa datos
   └─ Retorna resultado
   
4. ADAPTADOR (B) - responde si es necesario
   ├─ Convierte resultado a estructura compartida
   └─ Envía respuesta a A
   
5. PROTOCOLO
   ├─ Serializa respuesta
   └─ Envía por socket
```

## 📦 Estructuras Compartidas (utils/common/)

### CPU ↔ KERNEL
- `t_contexto_cpu`: Contexto de ejecución (PID, PC, registros, etc.)
- Motivos de desalojo: enum `t_motivo_desalojo`

### KERNEL ↔ MEMORIA
**Requests:**
- `t_mem_init_proceso`: PID + tamaño
- `t_mem_fin_proceso`: PID
- `t_mem_traducir`: PID + dirección lógica
- `t_mem_read`: PID + dirección + tamaño
- `t_mem_write`: PID + dirección + tamaño + buffer
- `t_mem_fetch`: PID + PC

**Responses:**
- `t_mem_respuesta_traduccion`: ok + dirección física
- `t_mem_respuesta_lectura`: ok + data + tamaño
- Respuesta genérica: int simple (1=OK, 0=FAIL)

### KERNEL ↔ IO
- `t_io_sleep`, `t_io_fs_create`, `t_io_fs_write`, etc.
- Respuesta: PID + éxito/fallo

## 🎯 Op_codes Organizados

### Gestión de Procesos (200-299: Kernel↔CPU)
- `OP_PROCESO_EXEC` (200): Kernel → CPU
- `OP_INTERRUPCION_CPU` (201): Kernel → CPU
- `OP_FIN_DE_QUANTUM` (202): CPU → Kernel
- `OP_CPU_FIN_PROCESO` (203): CPU → Kernel
- `OP_DESALOJO` (204): CPU → Kernel (incluye motivo)

### Memoria (300-399: Kernel/CPU ↔ Memoria)
- `OP_MEM_INIT_PROCESO` (300): Crear proceso
- `OP_MEM_FIN_PROCESO` (301): Finalizar proceso
- `OP_MEM_TRADUCIR_PAGINA` (302): Request
- `OP_MEM_RESP_TRADUCCION` (312): Response
- `OP_MEM_FETCH_INSTRUCCION` (303): CPU solicita
- `OP_MEM_RESP_INSTRUCCION` (313): Response
- `OP_MEM_LEER` (304): CPU solicita lectura
- `OP_MEM_RESP_LECTURA` (314): Response
- `OP_MEM_ESCRIBIR` (305): CPU solicita escritura
- `OP_MEM_RESP_ESCRITURA` (315): Response (1=OK, 0=FAIL)

### IO (400-499: Kernel ↔ IO)
- `OP_IO_SLEEP` (400): Esperar
- `OP_IO_FS_CREATE` (401): Crear archivo
- `OP_IO_FS_DELETE` (402): Eliminar
- `OP_IO_FS_WRITE` (403): Escribir
- `OP_IO_FS_READ` (404): Leer
- `OP_IO_FIN_OPERACION` (405): IO → Kernel: fin

### Genéricos (0-99)
- `OP_OK` (1): Respuesta exitosa
- `OP_FAIL` (0): Fallo
- `OP_HANDSHAKE` (99): Inicial

## 🛠️ Responsabilidades por Capa

### SERVIDOR (conexiones/server)
```c
while(recibir_paquete(socket)) {
    op_code = paquete->codigo_operacion;
    adaptador_procesar(paquete, op_code, socket);
    // El adaptador maneja TODO lo demás
}
```

### ADAPTADOR
```c
void adaptador_procesar(t_paquete* p, op_code code, int socket) {
    switch(code) {
        case OP_X:
            struct_x* req = recibir_x(p);  // Usa protocolo
            resultado = logica_interna(req);
            enviar_respuesta(socket, resultado);  // Usa protocolo
            free(req);
            break;
        // ...
    }
}
```

### PROTOCOLO/MENSAJES
```c
// Envío
void enviar_init_proceso(int socket, t_mem_init_proceso* req, op_code code) {
    t_paquete* p = paquete_create(code);
    paquete_write_uint32(p, req->pid);
    paquete_write_uint32(p, req->tamanio);
    enviar_paquete(socket, p);
    paquete_destroy(p);
}

// Recepción
t_mem_init_proceso* recibir_init_proceso(t_paquete* p) {
    t_mem_init_proceso* req = malloc(...);
    paquete_read_uint32(p, &req->pid);
    paquete_read_uint32(p, &req->tamanio);
    return req;
}
```

## 📁 Estructura de Directorios por Módulo

```
MODULO/
├── src/
│   ├── adaptadores/
│   │   ├── modulo_adapter.h      ← Todas las funciones de transformación
│   │   ├── modulo_adapter.c
│   │   └── *_responses.c         ← Envíos de respuestas (si necesario)
│   ├── conexiones/ O server/
│   │   ├── server.h
│   │   └── server.c              ← SOLO loop + delega a adaptador
│   ├── logica/
│   │   ├── gestion.c
│   │   └── procesamiento.c       ← Lógica interna pura
│   └── config/
│       ├── modulo_config.c
│       └── modulo_config.h
```

## ✅ Checklist de Validación

Cuando implementes comunicación entre módulos:

1. ✅ ¿Hay estructura compartida en `utils/common/`?
2. ✅ ¿Hay op_code definido en `protocolo/op_code.h`?
3. ✅ ¿Hay función `enviar_*` y `recibir_*` en `protocolo/mensajes.h/c`?
4. ✅ ¿El adaptador FORMA la estructura compartida?
5. ✅ ¿El adaptador SOLO usa funciones de protocolo/mensajes para enviar?
6. ✅ ¿El servidor SOLO recibe y delega?
7. ✅ ¿La lógica interna NO accede a protocolos/paquetes?
8. ✅ ¿Hay patrón consistente de request-response?

## 🔗 Ejemplos Concretos

### Ejemplo 1: Kernel → Memoria (Init Proceso)

**Paso 1: Kernel decide crear proceso**
```c
// kernel/src/logica/creacion.c
void crear_proceso_en_memoria(t_pcb* pcb) {
    // Delega a adaptador
    kernel_memoria_adapter_init_proceso(pcb);
}
```

**Paso 2: Adaptador prepara y envía**
```c
// kernel/src/adaptadores/kernel_memoria_adapter.c
void kernel_memoria_adapter_init_proceso(t_pcb* pcb) {
    // Convierte PCB a estructura compartida
    t_mem_init_proceso req = {
        .pid = pcb->pid,
        .tamanio = pcb->tam_proceso
    };
    
    // Usa protocolo para enviar
    enviar_init_proceso(socket_memoria, &req, OP_MEM_INIT_PROCESO);
    
    // Espera respuesta
    int respuesta = recibir_respuesta_simple(socket_memoria);
    if (respuesta == OP_OK) {
        log_info(logger, "Proceso %d iniciado en memoria", pcb->pid);
    }
}
```

**Paso 3: Memoria recibe**
```c
// memoria/src/server/server.c
case OP_MEM_INIT_PROCESO: {
    adaptador_procesar_init_proceso(paquete, cliente_socket);
    break;
}
```

**Paso 4: Adaptador de Memoria procesa**
```c
// memoria/src/adaptadores/memoria_adapter.c
void adaptador_procesar_init_proceso(t_paquete* p, int socket_kernel) {
    // Deserializa
    t_mem_init_proceso* req = recibir_init_proceso(p);
    
    // Convierte a estructura interna si es necesario
    // Llama lógica
    bool ok = memoria_crear_proceso(req->pid, req->tamanio);
    
    // Responde
    if (ok) {
        enviar_respuesta_ok(socket_kernel);
    } else {
        enviar_respuesta_fail(socket_kernel);
    }
    
    free(req);
}
```

## 🚀 Próximos Pasos

1. Completar `protocolo/mensajes.h/c` con todos los enviar/recibir
2. Implementar `t_*_adapter.c/h` para cada módulo
3. Refactorizar servidores para SOLO delegar
4. Validar flujos end-to-end (sin ejecutar, solo leyendo código)

