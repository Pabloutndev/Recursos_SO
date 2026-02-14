# 📟 Guía de Protocolos y OpCodes del Sistema
**Referencia de Comandos IPC - UTN FRBA**

Este documento detalla cada `op_code` definido en el sistema, su función técnica y su relación con la teoría de Sistemas Operativos aplicada en este proyecto.

---

## 1. Comandos Genéricos y Handshake
*Utilizados para la fase de inicio y validación de conexiones (Stallings Cap. 1).*

| OpCode | Función | Caso de Uso / Teoría |
| :--- | :--- | :--- |
| `OP_HANDSHAKE` | Validación inicial entre módulos. | Establecimiento de canal de comunicación IPC. |
| `OP_OK` / `OP_FAIL` | Respuestas de éxito o error para cualquier operación. | Manejo de excepciones y estados de retorno. |
| `OP_MENSAJE` / `OP_PAQUETE` | Envío de strings o estructuras genéricas. | Pruebas de conectividad y logs remotos. |

---

## 2. Kernel ↔ CPU (Ciclo de Ejecución)
*Controlan el despacho, desalojo y las interrupciones (Stallings Cap. 9 / Dino Cap. 6).*

| OpCode | Función | Caso de Uso / Teoría |
| :--- | :--- | :--- |
| `OP_PROCESO_EXEC` | Envía el Contexto (PCB simplificado) para ejecución. | **Despacho (Dispatch)**: Transición READY → EXEC. |
| `OP_INTERRUPCION_CPU` | Señal enviada por el socket de Interrupt. | **Desalojo Preemptivo**: Fin de Quantum o prioridad. |
| `OP_FIN_DE_QUANTUM` | CPU devuelve el proceso al vencer el tiempo. | **Round Robin**: Reencolado en READY. |
| `OP_CPU_FIN_PROCESO` | CPU devuelve el proceso porque llegó a `EXIT`. | **Terminación de Proceso**: Transición EXEC → EXIT. |
| `OP_SEGFAULT` | Error de acceso a memoria detectado por la MMU. | **Protección de Memoria**: Aborto de proceso. |
| `OP_DESALOJO` | CPU devuelve el proceso por un motivo genérico. | Desalojo forzoso por el Kernel. |

---

## 3. Kernel ↔ Memoria
*Gestión de la Memoria Virtual y el Ciclo de Vida (Stallings Cap. 8 / Dino Cap. 9).*

| OpCode | Función | Caso de Uso / Teoría |
| :--- | :--- | :--- |
| `OP_MEM_INIT_PROCESO` | Solicita crear estructuras para un nuevo proceso. | **Carga de Proceso**: Reserva de Tabla de Páginas. |
| `OP_MEM_FIN_PROCESO` | Solicita liberar espacio de un proceso finalizado. | **Liberación de Recursos**: Limpieza de RAM y Swap. |
| `OP_MEM_TRADUCIR_PAGINA` | Solicita el Marco (Frame) dada una Página. | **Unidad de Gestión de Memoria (MMU)**. |
| `OP_MEM_FETCH_INSTRUCCION`| CPU solicita la instrucción en el `PC` actual. | **Ciclo de Instrucción (Fetch)**. |
| `OP_MEM_LEER` / `ESCRIBIR` | Acceso a datos en la RAM física. | **Acceso a Memoria Principal**. |
| `OP_MEM_AJUSTAR_TAMANIO` | Instrucción `RESIZE`: Cambia tamaño del proceso. | **Asignación Dinámica**: Ajuste de tabla de páginas. |

---

## 4. Entrada/Salida y File System (DialFS)
*Gestión de bloqueos y dispositivos (Stallings Cap. 11-12 / Dino Cap. 13-15).*

| OpCode | Función | Caso de Uso / Teoría |
| :--- | :--- | :--- |
| `OP_IO_SLEEP` | El proceso solicita una espera genérica. | **Bloqueo de Proceso**: Transición EXEC → BLOCKED. |
| `OP_IO_FIN_OPERACION` | La interfaz avisa que terminó la tarea. | **Interrupción de E/S**: Transición BLOCKED → READY. |
| `OP_IO_FS_CREATE` / `DELETE` | Operaciones de creación y borrado de archivos. | **Gestión de Directorios (FS)**. |
| `OP_IO_FS_TRUNCATE` | Cambia el tamaño de un archivo físicamente. | **Asignación de Bloques (DialFS)**. |
| `OP_IO_FS_WRITE` / `READ` | Escribir o leer datos desde un archivo a Memoria. | **DMA Simulado**: Transferencia entre I/O y RAM. |
| `OP_WAIT_RECURSO` / `SIGNAL` | Pedido de semáforos compartidos entre procesos. | **Sincronización de Procesos**: Manejo de Mutexes. |

---

## 🔍 Análisis de Coherencia y Omisiones

### Conceptos "No Implementados" / Pendientes:
*   `OP_MEM_GET_SEGMENT` (no implementado): No se observan códigos para **Segmentación Pura**, ya que el sistema está volcado a Paginación.
*   `OP_CPU_SNAPSHOT` (no implementado): Falta un comando para volcado de estado de registros en caliente hacia el Kernel (útil para auditoría).
*   `OP_HANDSHAKE_PAGINA` (no implementado): No hay un código específico para que el Kernel o CPU reciban dinámicamente el `PAGE_SIZE` en el momento del Handshake, aunque se lee de config local.

### Diferencias con la Teoría:
*   **DMA**: Aunque el código `OP_IO_FS_WRITE/READ` existe, en este sistema es el Proceso de E/S quien accede a la Memoria, simulando el comportamiento de un controlador DMA real sin pasar por la CPU.
*   **Atención de Interrupciones**: El `OP_INTERRUPCION_CPU` se maneja de forma síncrona en el ciclo de instrucción (al chequear al inicio de cada ciclo), a diferencia de una interrupción de hardware real que dispararía una rutina instantánea (ISR).
