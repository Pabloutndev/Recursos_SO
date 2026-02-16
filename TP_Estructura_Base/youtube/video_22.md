# Video 22 - IO Genérica, STDIN y STDOUT

**Duración estimada:** 15 minutos  
**Bloque:** Entrada/Salida

---

## Conceptos

### Dispositivos de I/O
- **STDIN:** Teclado (entrada)
- **STDOUT:** Pantalla (salida)
- **Genérica:** Sleep, delays

### Blocking I/O
- Proceso se bloquea mientras espera
- Otros procesos pueden ejecutar

### Módulo EntradaSalida
- Proceso independiente
- Se registra en Kernel
- Ejecuta operaciones I/O

---

## Código

### Conexión y registro
**Archivo:** `entradasalida/src/core/io_main.c`

```c
int main(int argc, char** argv) {
    char* nombre_interfaz = argv[1];
    
    // Conectar al Kernel
    socket_kernel = crear_conexion(config.IP_KERNEL, config.PUERTO_KERNEL);
    
    // Registrarse
    t_paquete* paq = crear_paquete(IO_REGISTRO);
    escribir_string(paq, nombre_interfaz);
    escribir_string(paq, config.TIPO);  // GENERICA, STDIN, STDOUT, DIALFS
    enviar_paquete(paq, socket_kernel);
    
    log_info(logger, "Interfaz %s registrada", nombre_interfaz);
    
    // Loop principal
    while (true) {
        op_code op = recibir_operacion(socket_kernel);
        atender_request(op);
    }
}
```

### SLEEP
**Archivo:** `entradasalida/src/adaptadores/io_adapter.c`

```c
void handle_sleep(int socket) {
    t_paquete* paq = recibir_paquete(socket);
    uint32_t pid = leer_uint32(paq);
    uint32_t ms = leer_uint32(paq);
    
    log_info(logger, "Sleep: PID=%d, %d ms", pid, ms);
    
    // Simular I/O
    usleep(ms * 1000);
    
    // Notificar finalización
    t_paquete* respuesta = crear_paquete(IO_FIN);
    escribir_uint32(respuesta, pid);
    enviar_paquete(respuesta, socket);
    
    destruir_paquete(paq);
    destruir_paquete(respuesta);
}
```

### STDIN_READ
```c
void handle_stdin_read(int socket) {
    t_paquete* paq = recibir_paquete(socket);
    uint32_t pid = leer_uint32(paq);
    uint32_t dir_logica = leer_uint32(paq);
    uint32_t tamanio = leer_uint32(paq);
    
    log_info(logger, "STDIN Read: PID=%d, dir=%d, tam=%d", pid, dir_logica, tamanio);
    
    // Leer del teclado
    char buffer[256];
    printf("Ingrese datos para PID=%d: ", pid);
    fgets(buffer, tamanio + 1, stdin);
    
    // Escribir en Memoria del proceso
    solicitar_write_memoria(pid, dir_logica, buffer, strlen(buffer));
    
    // Notificar Kernel
    t_paquete* respuesta = crear_paquete(IO_FIN);
    escribir_uint32(respuesta, pid);
    enviar_paquete(respuesta, socket_kernel);
    
    destroir_paquete(paq);
}
```

### STDOUT_WRITE
```c
void handle_stdout_write(int socket) {
    t_paquete* paq = recibir_paquete(socket);
    uint32_t pid = leer_uint32(paq);
    uint32_t dir_logica = leer_uint32(paq);
    uint32_t tamanio = leer_uint32(paq);
    
    // Leer de Memoria del proceso
    void* datos = solicitar_read_memoria(pid, dir_logica, tamanio);
    
    // Mostrar por pantalla
    printf("[STDOUT PID=%d] %.*s\n", pid, (int)tamanio, (char*)datos);
    
    // Notificar Kernel
    t_paquete* respuesta = crear_paquete(IO_FIN);
    escribir_uint32(respuesta, pid);
    enviar_paquete(respuesta, socket_kernel);
    
    free(datos);
    destruir_paquete(paq);
}
```

---

## Lado Kernel

### Envío a I/O
```c
void kernel_io_sleep(t_pcb* pcb, char* interfaz, uint32_t ms) {
    int socket_io = obtener_socket_interfaz(interfaz);
    
    t_paquete* paq = crear_paquete(IO_SLEEP);
    escribir_uint32(paq, pcb->pid);
    escribir_uint32(paq, ms);
    enviar_paquete(paq, socket_io);
    
    // Bloquear proceso
    pcb->estado = ESTADO_BLOCKED;
    
    log_info(logger, "PID=%d bloqueado por IO_SLEEP en %s", pcb->pid, interfaz);
}
```

### Recepción de fin
```c
void handle_io_fin(int socket_io) {
    t_paquete* paq = recibir_paquete(socket_io);
    uint32_t pid = leer_uint32(paq);
    
    t_pcb* pcb = buscar_pcb(pid);
    
    pcb->estado = ESTADO_READY;
    queue_push(cola_ready, pcb);
    sem_post(&sem_hay_ready);
    
    log_info(logger, "I/O finalizada: PID=%d → READY", pid);
    
    destruir_paquete(paq);
}
```

---

## Demo: Test 2

### Proceso
```
SET AX 123
MOV_OUT 0 AX
IO_STDOUT_WRITE MONITOR 0 1
IO_GEN_SLEEP ESPERA 1000
EXIT
```

### Logs
```
[CPU] SET AX 123
[CPU] MOV_OUT 0 AX
[MEMORIA] Write: dir_log=0, valor=123
[CPU] IO_STDOUT_WRITE MONITOR 0 1
[KERNEL] PID=1 bloqueado por STDOUT
[IO MONITOR] STDOUT Write: PID=1, dir=0, tam=1
[IO MONITOR] Leyendo de Memoria...
[MEMORIA] Read: PID=1, dir=0 → 123
[IO MONITOR] [STDOUT PID=1] {
[KERNEL] PID=1 → READY (I/O fin)
[CPU] IO_GEN_SLEEP ESPERA 1000
[KERNEL] PID=1 bloqueado por SLEEP
[IO ESPERA] Sleep 1000ms...
... (1 segundo después)
[KERNEL] PID=1 → READY
[CPU] EXIT
```

---

## Puntos Clave

1. **Asíncrono:** I/O no bloquea todo el sistema
2. **Modular:** Cada interfaz es un proceso
3. **Registro:** Kernel conoce interfaces disponibles

---

## Siguiente Video

Veremos **DialFS: Filesystem** completo.
