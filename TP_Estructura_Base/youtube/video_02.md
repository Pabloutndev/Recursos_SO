# Video 2 - Protocolo de Comunicación

**Duración estimada:** 10-15 minutos  
**Bloque:** Fundamentos y Arquitectura

---

## Conceptos a Explicar

### IPC (Inter-Process Communication)
- Necesidad de comunicación entre procesos
- Diferentes mecanismos: pipes, memoria compartida, sockets
- Por qué elegimos sockets TCP

### Sockets TCP
- Conexión confiable, orientada a flujo
- Cliente-servidor: quién conecta y quién escucha
- Ventaja: cada módulo puede estar en máquinas diferentes

### Serialización
- Problema: enviar estructuras complejas por red
- Solución: convertir datos a bytes (serializar) y reconstruir (deserializar)
- Orden de bytes (endianness)

---

## Código y Demostración

### 1. Op Codes
**Archivo:** `utils/src/protocolo/op_code.h`

```c
// Rangos de opcodes por módulo
GENERICOS: 0-99
CPU: 100-199
MEMORIA: 200-299
IO: 300-399
KERNEL: 400-499
```

**Mostrar ejemplos:**
- `OP_HANDSHAKE = 1`
- `CPU_FETCH_INSTRUCCION = 100`
- `MEMORIA_CREAR_PROCESO = 200`
- `IO_SLEEP = 300`

### 2. Paquetes
**Archivo:** `utils/src/paquete/paquete.c`

**Mostrar funciones clave:**
```c
t_paquete* crear_paquete(op_code codigo);
void escribir_int32(t_paquete* paq, int32_t dato);
void escribir_string(t_paquete* paq, char* str);
void enviar_paquete(t_paquete* paq, int socket);
t_paquete* recibir_paquete(int socket);
```

**Estructura del paquete:**
```
[OP_CODE (4 bytes)] [TAMAÑO (4 bytes)] [DATOS (variable)]
```

### 3. Serialización
**Archivo:** `utils/src/serializacion/serializacion.c`

**Ejemplo: serializar contexto CPU**
```c
void serializar_contexto(t_paquete* paq, t_contexto* ctx) {
    escribir_uint32(paq, ctx->pid);
    escribir_uint32(paq, ctx->pc);
    // Serializar registros AX, BX, CX, DX...
    escribir_uint8(paq, ctx->registros.AX);
    // ...
}
```

### 4. Conexiones
**Archivo:** `utils/src/conexion/conexion.c`

**Cliente:**
```c
int socket_cliente = crear_conexion(ip, puerto);
if (socket_cliente < 0) {
    // Error de conexión
}
```

**Servidor:**
```c
int socket_servidor = iniciar_servidor(puerto);
int socket_cliente = esperar_cliente(socket_servidor);
```

**Handshake:**
```c
bool realizar_handshake(int socket, char* modulo) {
    t_paquete* paq = crear_paquete(OP_HANDSHAKE);
    escribir_string(paq, modulo);
    envidar_paquete(paq, socket);
    
    op_code respuesta = recibir_operacion(socket);
    return respuesta == OP_HANDSHAKE_OK;
}
```

---

## Demo: Flujo completo de un paquete

### Escenario: Consola envía comando RUN al Kernel

1. **Consola:**
   ```c
   t_paquete* paq = crear_paquete(RUN_PROCESO);
   escribir_string(paq, "test1.txt");
   enviar_paquete(paq, socket_kernel);
   ```

2. **Red:** bytes viajan por socket TCP

3. **Kernel:**
   ```c
   op_code op = recibir_operacion(socket_consola);
   if (op == RUN_PROCESO) {
       t_paquete* paq = recibir_paquete(socket_consola);
       char* path = leer_string(paq);
       ejecutar_proceso(path);
   }
   ```

**Mostrar logs:**
```
[CONSOLA] Enviando RUN test1.txt al Kernel
[KERNEL] Recibido RUN_PROCESO: test1.txt
```

---

## Puntos Clave a Destacar

1. **Abstracción:** La librería `utils` encapsula toda la complejidad
2. **Extensibilidad:** Agregar nuevos tipos de mensajes es agregar un op_code
3. **Robustez:** Manejo de errores en conexiones
4. **Depuración:** Logs detallados de cada comunicación

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos la **Consola** en acción
- Interfaz REPL para enviar comandos
- Comandos disponibles: RUN, KILL, PS, etc.
