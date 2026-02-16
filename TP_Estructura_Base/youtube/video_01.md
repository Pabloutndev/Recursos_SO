# Video 1 - Introducción al Sistema Operativo

**Duración estimada:** 15-20 minutos  
**Bloque:** Fundamentos y Arquitectura

---

## Conceptos a Explicar

### ¿Qué es un Sistema Operativo?
- Intermediario entre hardware y aplicaciones
- Gestor de recursos: CPU, memoria, dispositivos I/O
- Abstracción del hardware para facilitar programación

### ¿Por qué existe un SO?
- Problemas que resuelve:
  - Gestión eficiente de recursos limitados
  - Multiprogramación: varios procesos simultaneos
  - Protección y aislamiento entre procesos
  - Interfaz de alto nivel para desarrolladores

### Componentes principales del SO
- Kernel (núcleo)
- Planificador de procesos
- Gestor de memoria
- Sistema de I/O
- Sistema de archivos

---

## Código y Demostración

### 1. Arquitectura del TP
**Mostrar:** Diagrama de los 5 módulos
- **Kernel:** Coordina todo el sistema, planifica procesos
- **CPU:** Ejecuta instrucciones de los procesos
- **Memoria:** Gestiona RAM y swap
- **IO (EntradaSalida):** Maneja dispositivos de entrada/salida
- **Consola:** Interfaz de usuario para enviar comandos

### 2. Comunicación entre módulos
**Mostrar:** Diagrama de conexiones TCP
- Sockets TCP para IPC (Inter-Process Communication)
- Cada módulo es un proceso independiente
- Protocolo de mensajes serializado

### 3. Flujo de vida de un proceso
**Mostrar:** `PROYECTO.md`
```
NEW → READY → EXEC → BLOCKED → EXIT
```
- **NEW:** Proceso recién creado
- **READY:** Listo para ejecutar, esperando CPU
- **EXEC:** Ejecutando en CPU
- **BLOCKED:** Esperando I/O o recurso
- **EXIT:** Finalizado

### 4. Recorrido del repositorio
**Mostrar en terminal:**
```
TP_Estructura_Base/
├── kernel/
├── cpu/
├── memoria/
├── entradasalida/
├── consola/
├── utils/          # Biblioteca compartida
├── tests/          # Suite de pruebas
└── Makefile
```

**Demostrar:**
```bash
# Compilar todos los módulos
make all

# Ejecutar módulos individualmente
./bin/memoria memoria.config
./bin/cpu cpu.config
./bin/kernel kernel.config
```

---

## Puntos Clave a Destacar

1. **Modularidad:** Cada componente tiene responsabilidad única
2. **Comunicación asíncrona:** Mensajes a través de sockets
3. **Estado del proceso:** El SO rastrea todo proceso en todo momento
4. **Simplicidad didáctica:** Versión simplificada de un SO real

---

## Preparación para el siguiente video

Mencionar que en el próximo video:
- Veremos **cómo se comunican** estos módulos
- Protocolo de serialización de mensajes
- Handshake y establecimiento de conexiones
