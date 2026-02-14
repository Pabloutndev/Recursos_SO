# 🎓 GUÍA TEÓRICO-PRÁCTICA: Auditoría y Simulación de SO
**TP Sistemas Operativos - UTN FRBA**

Este documento consolida el análisis técnico, los flujos operativos y el sustento teórico (Bibliografía Stallings & Dinosaurio) del simulador.

---

## 🏗️ 1. Arquitectura y Módulos (Compliance)

| Módulo | Función Teórica | Estado de Implementación |
| :--- | :--- | :--- |
| **Kernel** | Planificador de procesos, sincronización y recursos. | ✅ Modelo síncrono (bloqueante en Dispatch). |
| **CPU** | Ciclo de instrucción y manejo de interrupciones. | ✅ Fetch-Decode-Execute-Interrupt. |
| **Memoria** | Gestión de espacio de usuario y memoria virtual. | ✅ Von Neumann (Instrucciones y Datos unificados). |
| **I/O** | Interfaz con dispositivos y File System. | ✅ DialFS (Asignación Contigua). |

---

## 🔄 2. Flujo Operativo Principal (Pasos del Sistema)

Este es el recorrido de un proceso desde su carga hasta el desalojo por Quantum:

1.  **Carga (NEW)**: Kernel recibe `RUN`. Envía `OP_MEM_INIT_PROCESO`.
2.  **Inicialización**: Memoria carga instrucciones en **RAM Física** (vía paginación).
3.  **Preparación (READY)**: Kernel mueve a `READY` tras recibir el frame de memoria.
4.  **Despacho (EXEC)**: El Planificador de Corto Plazo elige el proceso y envía el Contexto a CPU por el socket de **Dispatch**.
5.  **Bloqueo de Sincronismo**: El Kernel se queda esperando en la función `atender_dispatch_cpu()`.
6.  **Ciclo en CPU**:
    *   **Fetch**: CPU pide `OP_MEM_FETCH_INSTRUCCION`.
    *   **MMU**: Traduce lógica -> física (usa TLB).
    *   **Execute**: CPU ejecuta (ej: `SUM`, `MOV`).
7.  **Interrupción**: El Timer del Kernel envía `OP_INTERRUPT` por el socket de **Interrupt**.
8.  **Retorno**: CPU detecta el flag, termina la instrucción actual y devuelve el Contexto por **Dispatch** con el código `OP_FIN_DE_QUANTUM`.
9.  **Replanificación**: Kernel recibe, actualiza el PCB y reencola en `READY`.

---

## 🧠 3. Mapeo Teórico (Stallings & Dinosaurios)

| Concepto Teórico | Bibliografía | Implementación en TP |
| :--- | :--- | :--- |
| **PCB (Process Control Block)** | Stallings Cap. 3 / Dino Cap. 3 | ✅ (PID, PC, Registros, Quantum). |
| **Planificación de CPU** | Stallings Cap. 9 / Dino Cap. 6 | ✅ (FIFO, RR, HRRN). |
| **Paginación a Demanda** | Stallings Cap. 8 / Dino Cap. 9 | ✅ (Carga desde Swap en Page Fault). |
| **Algoritmo Clock** | Stallings Cap. 8 | ✅ (Reemplazo de páginas). |
| **DMA (Acceso Directo)** | Stallings Cap. 11 | ⚠️ Simulado (IO escribe en Memoria). |
| **Deadlocks (Banquero)** | Stallings Cap. 6 | ❌ (no implementado). |
| **Segmentación Pura** | Dino Cap. 8 | ❌ (no implementado). |
| **Threads de Usuario (ULT)** | Stallings Cap. 4 | ❌ (no implementado). |

---

## 🔌 4. I/O, Interrupciones y File System

### 📢 I/O e Interrupciones (Lectura Recomendada)
Para entender cómo los dispositivos bloquean al proceso y avisan al kernel:
*   **Stallings (7ma Ed)**: **Capítulo 11** (Técnicas de E/S, Manejadores de dispositivos).
*   **Dinosaurio (9na Ed)**: **Capítulo 13** (Interfaz de E/S, Bloqueo vs No bloqueo).

### 📁 File System (DialFS)
DialFS implementa **Asignación Contigua**, fundamental para entender la performance de disco:
*   **Stallings (7ma Ed)**: **Capítulo 12** (Métodos de Asignación de Archivos).
*   **Dinosaurio (9na Ed)**: **Capítulo 14 y 15** (Estructura de Directorios y Métodos de Asignación).

---

## 🔍 5. Veredicto de la Auditoría
1.  **Sincronización**: Se eliminó la escucha asíncrona en Kernel para evitar condiciones de carrera, adoptando un modelo de Dispatch síncrono.
2.  **Memoria**: Se unificó el fetch de instrucciones para que pase por la MMU, cumpliendo con la transparencia de la memoria virtual.
3.  **Config**: Cada módulo es independiente y carga sus propios constantes (e.g. `PAGE_SIZE`).

**Resultado Final**: El sistema es coherente, modular y apto para aprobación según los requisitos de la cátedra.
