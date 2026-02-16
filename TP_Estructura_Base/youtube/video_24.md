# Video 24 - Test Suite: Cómo Verificar el SO

**Duración estimada:** 10-15 minutos  
**Bloque:** Testing y Cierre

---

## Conceptos

### Testing basado en logs
- Verificar comportamiento por logs
- Patrones esperados
- Automatización con scripts

### Suite de tests
- 11 tests automatizados
- Cubren todos los componentes
- Scripts Bash para orquestación

---

## Estructura

### Archivos de test
```
tests/
├── run_all.sh              # Ejecutar todos
├── start_modules.sh        # Levantar módulos
├── test_01_ciclo_basico.sh
├── test_02_io.sh
├── test_03_io_multiple.sh
├── test_04_quantum.sh
├── test_05_recursos.sh
├── test_06_memoria.sh
├── test_10_prioridades.sh
├── test_11_deadlock.sh
└── ...
```

### Funciones helper
**Archivo:** `tests/utils.sh`

```bash
wait_for_log() {
    local log_file=$1
    local pattern=$2
    local timeout=$3
    
    local elapsed=0
    while ! grep -q "$pattern" "$log_file"; do
        sleep 0.5
        elapsed=$((elapsed + 1))
        
        if [ $elapsed -ge $timeout ]; then
            echo "TIMEOUT esperando: $pattern"
            return 1
        fi
    done
    
    echo "✓ Encontrado: $pattern"
    return 0
}

wait_for_log_count() {
    local log_file=$1
    local pattern=$2
    local count=$3
    
    while [ $(grep -c "$pattern" "$log_file") -lt $count ]; do
        sleep 0.5
    done
    
    echo "✓ $count ocurrencias de: $pattern"
}

mark_log() {
    local log_file=$1
    local marker="===== TEST MARKER $(date +%s) ====="
    echo "$marker" >> "$log_file"
}
```

---

## Ejemplo: Test 1 (Ciclo Básico)

```bash
#!/bin/bash

source tests/utils.sh

echo "=== Test 1: Ciclo Básico ==="

# Limpiar logs
> kernel.log
> cpu.log
> memoria.log

# Iniciar módulos en background
./bin/memoria memoria.config &
PID_MEMORIA=$!

./bin/cpu cpu.config &
PID_CPU=$!

./bin/kernel kernel.config &
PID_KERNEL=$!

sleep 2  # Esperar inicialización

# Ejecutar prueba
echo "RUN test1.txt" | ./bin/consola consola.config

# Verificaciones
wait_for_log "kernel.log" "Proceso creado: PID=1" 5 || exit 1
wait_for_log "memoria.log" "Proceso PID=1 creado" 5 || exit 1
wait_for_log "cpu.log" "Ejecutando EXIT" 10 || exit 1
wait_for_log "kernel.log" "Proceso PID=1 finalizó" 10 || exit 1

# Cleanup
kill $PID_MEMORIA $PID_CPU $PID_KERNEL

echo "✓ Test 1: PASADO"
```

---

## Recorrido de Tests

### Test 1: Ciclo Básico
- NEW → READY → EXEC → EXIT
- Instrucciones simples (SET, SUM)

### Test 2: I/O Bloqueante
- IO_GEN_SLEEP
- Transición EXEC → BLOCKED → READY

### Test 3: I/O Múltiple
- Varios dispositivos
- STDIN_READ, STDOUT_WRITE

### Test 4: Quantum
- Round Robin
- Desalojo por quantum

### Test 5: Recursos
- WAIT/SIGNAL
- Un proceso bloquea, otro desbloquea

### Test 6: Memoria
- MOV_IN, MOV_OUT, RESIZE
- Traducción de direcciones

### Test 10: Prioridades
- Algoritmo de prioridades
- PAUSE/START

### Test 11: Deadlock
- Dos procesos en abrazo mortal
- Detección con grafo

---

## Scripts de proceso

**Ubicación:** `memoria/procesos/`

### test1.txt (básico)
```
SET AX 10
SET BX 20
SUM AX BX
EXIT
```

### test_io.txt
```
SET AX 5
IO_GEN_SLEEP ESPERA 1000
SET BX 10
EXIT
```

### infinito.txt (para quantum)
```
SET AX 0
SUM AX AX
SUM AX AX
... (cientos de líneas)
```

### deadlock_a.txt
```
WAIT RA
WAIT RB
SIGNAL RB
SIGNAL RA
EXIT
```

---

## Ejecutar Suite Completa

```bash
$ cd tests
$ ./run_all.sh

=== Test 1: Ciclo Básico ===
✓ Proceso creado
✓ Instrucciones ejecutadas
✓ Proceso finalizado
✓ Test 1: PASADO

=== Test 2: I/O ===
✓ Bloqueado por I/O
✓ I/O completada
✓ Test 2: PASADO

...

=== Resumen ===
11/11 tests PASADOS
```

---

## Puntos Clave

1. **Automatización:** Scripts ahorran tiempo
2. **Logs:** Fuente de verdad para verificación
3. **Coverage:** Tests cubren todos los features

---

## Siguiente Video

Veremos **Recapitulación: El SO Completo**.
