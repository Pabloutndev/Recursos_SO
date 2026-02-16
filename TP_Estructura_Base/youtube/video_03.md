# Video 3 - La Consola: Interfaz de Usuario

**Duración estimada:** 5-10 minutos  
**Bloque:** Fundamentos y Arquitectura

---

## Conceptos a Explicar

### Interfaz Humano-Máquina
- El SO necesita una forma de recibir comandos del usuario
- Shell vs GUI: línea de comandos vs interfaz gráfica
- Ventaja de CLI: automatización, scripting

### REPL (Read-Eval-Print Loop)
- **Read:** Leer entrada del usuario
- **Eval:** Interpretar y ejecutar comando
- **Print:** Mostrar resultado
- **Loop:** Repetir indefinidamente

### Comandos del sistema
- Comandos básicos de gestión de procesos
- Comandos de planificación
- Comandos de debugging

---

## Código y Demostración

### 1. Loop principal
**Archivo:** `consola/src/consola.c`

```c
int main() {
    conectar_al_kernel();
    
    while (true) {
        char* linea = readline("so> ");
        if (!linea) break;
        
        ejecutar_comando(linea);
        add_history(linea);
        free(linea);
    }
}
```

**Explicar:**
- `readline()`: librería GNU que permite edición de línea, historial
- `add_history()`: guarda comando para reutilizar con ↑
- Loop infinito hasta EOF (Ctrl+D) o comando EXIT

### 2. Parseo de comandos
**Archivo:** `consola/src/helpers/consola_helpers.c`

```c
typedef struct {
    char* nombre;
    int argc;  // argumentos requeridos
    void (*handler)(char** args);
} t_comando;

t_comando TABLA_COMANDOS[] = {
    {"RUN",       1, cmd_run},
    {"KILL",      1, cmd_kill},
    {"PS",        0, cmd_ps},
    {"ALGORITMO", 1, cmd_algoritmo},
    {"START",     0, cmd_start},
    {"PAUSE",     0, cmd_pause},
    {"EXIT",      0, cmd_exit}
};
```

**Función de parseo:**
```c
void ejecutar_comando(char* linea) {
    char** tokens = string_split(linea, ' ');
    char* cmd = tokens[0];
    
    t_comando* comando = buscar_comando(cmd);
    if (comando) {
        if (validar_argumentos(tokens, comando->argc)) {
            comando->handler(tokens + 1);
        }
    } else {
        printf("Comando desconocido: %s\n", cmd);
    }
}
```

### 3. Adaptador Kernel
**Archivo:** `consola/src/adaptadores/consola_kernel_adapter.c`

```c
void cmd_run(char** args) {
    char* path = args[0];
    
    t_paquete* paq = crear_paquete(RUN_PROCESO);
    escribir_string(paq, path);
    enviar_paquete(paq, socket_kernel);
    
    log_info(logger, "Enviado RUN: %s", path);
}

void cmd_kill(char** args) {
    int pid = atoi(args[0]);
    
    t_paquete* paq = crear_paquete(KILL_PROCESO);
    escribir_int32(paq, pid);
    enviar_paquete(paq, socket_kernel);
}

void cmd_ps(char** args) {
    t_paquete* paq = crear_paquete(LISTAR_PROCESOS);
    enviar_paquete(paq, socket_kernel);
    
    // Esperar respuesta del kernel
    t_paquete* respuesta = recibir_paquete(socket_kernel);
    imprimir_lista_procesos(respuesta);
}
```

---

## Demo en Vivo

### Ejecutar la consola y probar comandos

```bash
$ ./bin/consola consola.config
Conectado al Kernel en 127.0.0.1:8001
so> RUN test1.txt
[OK] Proceso creado con PID: 1
so> PS
PID | Estado  | Path
1   | READY   | test1.txt
so> START
[OK] Planificación iniciada
so> PAUSE
[OK] Planificación pausada
so> KILL 1
[OK] Proceso 1 finalizado
so> ALGORITMO RR
[OK] Algoritmo cambiado a Round Robin
so> EXIT
Desconectando...
```

---

## Comandos Detallados

### RUN `<path>`
- Crea un nuevo proceso a partir del archivo de instrucciones
- El Kernel lo coloca en estado NEW

### KILL `<pid>`
- Finaliza forzosamente un proceso
- Libera recursos, limpia memoria

### PS
- Lista todos los procesos activos
- Muestra PID, estado, path

### ALGORITMO `<tipo>`
- Cambia el algoritmo de planificación dinámicamente
- Opciones: FIFO, RR, VRR, HRRN, PRIORIDAD

### START / PAUSE
- Inicia o pausa la planificación de largo plazo
- Útil para debugging: crear procesos sin ejecutarlos

### EXIT
- Cierra la consola
- No afecta al Kernel (sigue corriendo)

---

## Puntos Clave a Destacar

1. **Simplicidad:** Internamente solo envía paquetes al Kernel
2. **Desacoplamiento:** La consola puede cerrarse y reabrise sin afectar el SO
3. **Extensibilidad:** Agregar nuevos comandos es agregar entrada a la tabla
4. **UX:** Historial de comandos, autocompletado (readline)

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos qué pasa **dentro del Kernel** cuando llega un RUN
- Estructura de datos fundamental: el **PCB**
- Estados de un proceso en detalle
