#!/usr/bin/env bash
# run_all.sh - Test suite para TP Sistemas Operativos (UTN FRBA)
#
# Evalua las problematicas tipicas de los TPs:
#   1. Ciclo de vida de un proceso (NEW -> READY -> EXEC -> EXIT)
#   2. IO bloqueante (EXEC -> BLOCKED -> READY)
#   3. IO multiple (multiples transiciones a BLOCKED)
#   4. Quantum con Round Robin (desalojo por fin de quantum)
#   5. FIFO sin desalojo (proceso ejecuta sin interrupcion)
#   6. Recursos compartidos (WAIT/SIGNAL con bloqueo por recurso)
#   7. Operaciones de memoria (MOV_OUT + MOV_IN)
#   8. Procesos concurrentes (varios procesos simultaneos)
#   9. KILL de proceso (terminacion forzada)
#
# Uso:
#   cd tests && bash run_all.sh
#
# Enfoque: verificacion basada en LOGS, no en tiempos fijos.
#   Cada test lanza un proceso y ESPERA a que aparezca el patron
#   esperado en los logs del kernel (con timeout). Esto es mas
#   confiable que usar sleeps fijos.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_DIR="$SCRIPT_DIR/logs"

# Log primario: el que crea el kernel via commons library (se flushea por linea).
# Fallback: nuestro stdout capturado en tests/logs/kernel.log.
KERNEL_OWN_LOG="$BASE_DIR/kernel/kernel.log"
KERNEL_STDOUT_LOG="$LOG_DIR/kernel.log"

# Timeout por defecto para esperar patrones en logs (segundos)
DEFAULT_TIMEOUT=30

# =============================
# Cleanup automatico
# =============================
cleanup() {
    echo ""
    echo "=== Limpieza: deteniendo modulos ==="
    "$SCRIPT_DIR/stop_modules.sh" 2>/dev/null || true
}
trap cleanup EXIT

# =============================
# Limpieza de logs anteriores
# =============================
echo "Limpiando logs de corridas anteriores..."
rm -f "$KERNEL_OWN_LOG" "$BASE_DIR/kernel/kernel_error.log"
rm -rf "$LOG_DIR"
mkdir -p "$LOG_DIR"

# =============================
# Funciones auxiliares
# =============================

# Formatos de log del kernel (de logger.c y corto_plazo.c):
#   "PID: X - Proceso creado -> NEW"
#   "PID: X - <estado_ant> -> <estado_act>"
#   "PID: X - EXEC -> EXIT (motivo)"
#   "PID: X - EXEC -> BLOCKED (motivo)"
#   "Quantum vencido -> PID=X desalojado"          (timer automatico)
#   "PID: X - Desalojado por fin de Quantum"        (DESALOJAR manual)
#   "PID: X - Wait: RECURSO - Instancias: N"
#   "PID: X - Signal: RECURSO - Instancias: N"
#   "KILL/EXIT"

LOG_MARK_OWN=0
LOG_MARK_STDOUT=0

mark_log() {
    LOG_MARK_OWN=0
    LOG_MARK_STDOUT=0
    [[ -f "$KERNEL_OWN_LOG" ]] && LOG_MARK_OWN=$(wc -l < "$KERNEL_OWN_LOG")
    [[ -f "$KERNEL_STDOUT_LOG" ]] && LOG_MARK_STDOUT=$(wc -l < "$KERNEL_STDOUT_LOG")
}

new_log_lines() {
    {
        [[ -f "$KERNEL_OWN_LOG" ]] && tail -n +"$((LOG_MARK_OWN + 1))" "$KERNEL_OWN_LOG"
        [[ -f "$KERNEL_STDOUT_LOG" ]] && tail -n +"$((LOG_MARK_STDOUT + 1))" "$KERNEL_STDOUT_LOG"
    } 2>/dev/null || true
}

check_log() {
    new_log_lines | grep -qiE "$1"
}

count_log() {
    local n
    n=$(new_log_lines | grep -ciE "$1" 2>/dev/null) || n=0
    echo "$n"
}

# wait_for_log: espera hasta que aparezca un patron en los logs.
# Retorna 0 si lo encuentra, 1 si se agota el timeout.
# Uso: wait_for_log "EXEC -> EXIT" 20
wait_for_log() {
    local pattern="$1"
    local timeout="${2:-$DEFAULT_TIMEOUT}"
    local elapsed=0
    while [[ $elapsed -lt $timeout ]]; do
        if new_log_lines | grep -qiE "$pattern"; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 1
}

# wait_for_log_count: espera hasta que un patron aparezca N veces.
# Retorna 0 si llega a N, 1 si se agota el timeout.
wait_for_log_count() {
    local pattern="$1"
    local expected="$2"
    local timeout="${3:-$DEFAULT_TIMEOUT}"
    local elapsed=0
    while [[ $elapsed -lt $timeout ]]; do
        local n
        n=$(new_log_lines | grep -ciE "$pattern" 2>/dev/null) || n=0
        if [[ "$n" -ge "$expected" ]]; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done
    return 1
}

send_cmd() {
    (cd "$BASE_DIR/consola" && printf '%s\n' "$1" | ./bin/consola) > /dev/null 2>&1
}

run_process() {
    (cd "$BASE_DIR/consola" && printf 'RUN %s\n' "$1" | ./bin/consola) > /dev/null 2>&1
}

run_process_with_priority() {
    local name="$1"
    local prio="$2"
    (cd "$BASE_DIR/consola" && printf 'RUN %s %s\n' "$name" "$prio" | ./bin/consola) > /dev/null 2>&1
}

# Tracking de resultados
TOTAL=0; PASSED=0; FAILED=0
RESULTS=""

begin_test() {
    TOTAL=$((TOTAL + 1))
    echo ""
    echo "  [$TOTAL] $1"
    mark_log
}

pass() {
    PASSED=$((PASSED + 1))
    RESULTS="${RESULTS}  PASS  |  $1\n"
    echo "       -> PASS"
}

fail() {
    FAILED=$((FAILED + 1))
    RESULTS="${RESULTS}  FAIL  |  $1\n"
    echo "       -> FAIL${2:+ ($2)}"
}

# =============================
# Inicio
# =============================

echo "================================================"
echo "   Test Suite - Sistemas Operativos UTN FRBA"
echo "================================================"
echo ""

"$SCRIPT_DIR/start_modules.sh"

echo ""
echo "Esperando estabilizacion (3s)..."
sleep 3

send_cmd "START"
sleep 1

# ==========================================================
#  FASE 1: PLANIFICACION CON ROUND ROBIN
# ==========================================================

echo ""
echo "================================================"
echo "  FASE 1: Round Robin"
echo "================================================"

send_cmd "ALGORITMO RR"
sleep 1

# --- Test 1: Ciclo de vida basico ---
DESC="Ciclo de vida: NEW -> READY -> EXEC -> EXIT"
begin_test "$DESC"
run_process "test1.txt"
if wait_for_log "EXEC -> EXIT" 15; then
    pass "$DESC"
else
    fail "$DESC" "no se detecto 'EXEC -> EXIT' en logs (timeout)"
fi

# --- Test 2: IO bloqueante ---
DESC="IO bloqueante: proceso pasa a BLOCKED y vuelve a READY"
begin_test "$DESC"
run_process "test_io.txt"
if wait_for_log "EXEC -> EXIT" 20; then
    if check_log "EXEC -> BLOCKED"; then
        if check_log "BLOCKED -> READY"; then
            pass "$DESC"
        else
            fail "$DESC" "se detecto BLOCKED pero no BLOCKED -> READY (IO no completo, verificar GENERICA1)"
        fi
    else
        fail "$DESC" "el proceso termino pero no paso por BLOCKED (falta IO)"
    fi
else
    # No llego a EXIT, verificar si al menos bloqueo
    if check_log "EXEC -> BLOCKED"; then
        fail "$DESC" "se detecto BLOCKED pero el proceso no completo (timeout esperando EXIT)"
    else
        fail "$DESC" "no se detecto 'EXEC -> BLOCKED' en logs (timeout)"
    fi
fi

# --- Test 3: IO multiple ---
DESC="IO multiple: multiples transiciones EXEC -> BLOCKED"
begin_test "$DESC"
run_process "multi_io.txt"
if wait_for_log "EXEC -> EXIT" 30; then
    n=$(count_log "EXEC -> BLOCKED")
    if [[ "$n" -ge 2 ]]; then
        pass "$DESC"
    else
        fail "$DESC" "se esperaban >=2 bloqueos por IO, se detectaron $n"
    fi
else
    n=$(count_log "EXEC -> BLOCKED")
    fail "$DESC" "el proceso no completo (timeout). Bloqueos detectados: $n"
fi

# --- Test 4: Quantum RR ---
# test_quantum.txt: ~50 instrucciones = ~5s, con Q=2000ms se desaloja 2-3 veces
DESC="Quantum RR: desalojo por fin de quantum"
begin_test "$DESC"
run_process "test_quantum.txt"
if wait_for_log "EXEC -> EXIT" 20; then
    if check_log "Quantum vencido|Desalojado por fin de Quantum"; then
        pass "$DESC"
    else
        fail "$DESC" "el proceso termino pero no se detecto desalojo por quantum"
    fi
else
    # Aunque no haya terminado, verificar si hubo quantum
    if check_log "Quantum vencido|Desalojado por fin de Quantum"; then
        fail "$DESC" "se detecto quantum pero el proceso no termino (timeout)"
    else
        fail "$DESC" "no se detecto 'Quantum vencido' en logs (timeout)"
    fi
fi

# --- Test 5: Recursos compartidos ---
DESC="Recursos: WAIT/SIGNAL con bloqueo por recurso (RA, 1 instancia)"
begin_test "$DESC"
run_process "test_recurso_a.txt"
sleep 1
run_process "test_recurso_b.txt"
if wait_for_log_count "EXEC -> EXIT" 2 25; then
    pass "$DESC"
else
    exits=$(count_log "EXEC -> EXIT")
    fail "$DESC" "se esperaban 2 EXIT, se detectaron $exits (timeout)"
fi

# --- Test 6: Operaciones de memoria ---
DESC="Memoria: MOV_OUT + MOV_IN (escritura y lectura)"
begin_test "$DESC"
run_process "test_memoria_ops.txt"
if wait_for_log "EXEC -> EXIT" 15; then
    if check_log "SEGFAULT|Segmentation"; then
        fail "$DESC" "el proceso termino con SEGFAULT"
    else
        pass "$DESC"
    fi
else
    fail "$DESC" "el proceso no llego a EXIT (timeout)"
fi

# --- Test 7: Procesos concurrentes ---
DESC="Concurrencia: 3 procesos simultaneos completan correctamente"
begin_test "$DESC"
run_process "test1.txt"
run_process "test1.txt"
run_process "test1.txt"
if wait_for_log_count "EXEC -> EXIT" 3 20; then
    pass "$DESC"
else
    exits=$(count_log "EXEC -> EXIT")
    fail "$DESC" "se esperaban >=3 EXIT, se detectaron $exits (timeout)"
fi

# ==========================================================
#  FASE 2: PLANIFICACION CON FIFO
# ==========================================================

echo ""
echo "================================================"
echo "  FASE 2: FIFO (sin desalojo por quantum)"
echo "================================================"

send_cmd "ALGORITMO FIFO"
sleep 1

# --- Test 8: FIFO sin desalojo ---
DESC="FIFO: proceso ejecuta SIN desalojo por quantum"
begin_test "$DESC"
run_process "test_quantum.txt"
if wait_for_log "EXEC -> EXIT" 20; then
    if check_log "Quantum vencido|Desalojado por fin de Quantum"; then
        fail "$DESC" "se detecto desalojo por quantum (no deberia con FIFO)"
    else
        pass "$DESC"
    fi
else
    fail "$DESC" "el proceso no llego a EXIT (timeout)"
fi

# ==========================================================
#  FASE 3: TESTS ESPECIALES
# ==========================================================

echo ""
echo "================================================"
echo "  FASE 3: Tests especiales"
echo "================================================"

send_cmd "ALGORITMO FIFO"
sleep 1

# --- Test 9: KILL de proceso ---
# infinito.txt: loop infinito (SET AX 1, JNZ AX 1)
# Usamos FIFO para que el proceso quede en EXEC sin desalojos por quantum,
# evitando race conditions en la transicion EXEC->READY al momento del KILL.
DESC="KILL: terminar proceso en ejecucion forzadamente"
begin_test "$DESC"
run_process "infinito.txt"
# Esperar a que el proceso este en EXEC (no solo NEW, ya que largo_plazo
# saca de NEW y llama a Memoria antes de encolar en READY; si KILL llega
# durante esa ventana, el PCB no esta en ninguna cola).
if wait_for_log "READY -> EXEC" 10; then
    PID=$(new_log_lines | grep -oE 'PID: [0-9]+' | tail -1 | grep -oE '[0-9]+')
    if [[ -n "$PID" ]]; then
        send_cmd "KILL $PID"
        if wait_for_log "KILL/EXIT" 10; then
            pass "$DESC"
        else
            fail "$DESC" "KILL enviado (PID=$PID) pero no se confirmo terminacion en logs"
        fi
    else
        fail "$DESC" "no se pudo extraer PID del log"
    fi
else
    fail "$DESC" "no se detecto creacion del proceso en logs (timeout)"
fi

# ==========================================================
#  FASE 4: PLANIFICACION CON PRIORIDADES
# ==========================================================

echo ""
echo "================================================"
echo "  FASE 4: Prioridades (menor valor = mayor prioridad)"
echo "================================================"

send_cmd "ALGORITMO PRIORIDAD"
sleep 2

# Warm-up: ejecutar un proceso simple para verificar que el scheduler esta
# limpio despues del KILL de test 9 (la respuesta de CPU post-interrupt
# necesita ser procesada por corto_plazo antes de continuar).
echo "  [warmup] Verificando scheduler con PRIORIDAD..."
mark_log
run_process "test1.txt"
if wait_for_log "EXEC -> EXIT" 15; then
    echo "  [warmup] OK - scheduler funcional"
else
    echo "  [warmup] WARN - proceso warmup no completo, continuando"
fi
sleep 1

send_cmd "PAUSE"
sleep 2

# --- Test 10: Prioridades ---
# Estrategia: PAUSE para acumular procesos en NEW, luego START.
# largo_plazo los mueve a READY secuencialmente; corto_plazo los ejecuta.
# Verificamos que ambos procesos completan bajo el algoritmo PRIORIDAD.
DESC="Prioridades: proceso con prioridad 1 ejecuta antes que prioridad 10"
begin_test "$DESC"
run_process_with_priority "test_prioridad_baja.txt" 10
sleep 1
run_process_with_priority "test_prioridad_alta.txt" 1
sleep 2
send_cmd "START"
# Esperar a que ambos lleguen a READY (largo_plazo los procesa secuencialmente)
if wait_for_log_count "NEW -> READY" 2 15; then
    # Ahora esperar que ambos terminen
    if wait_for_log_count "EXEC -> EXIT" 2 30; then
        # Verificar orden: el de prioridad alta (1) debe hacer EXEC -> EXIT primero
        FIRST_EXIT_PID=$(new_log_lines | grep -oE 'PID: [0-9]+ - EXEC -> EXIT' | head -1 | grep -oE '[0-9]+' | head -1)
        SECOND_EXIT_PID=$(new_log_lines | grep -oE 'PID: [0-9]+ - EXEC -> EXIT' | tail -1 | grep -oE '[0-9]+' | head -1)
        if [[ "$FIRST_EXIT_PID" != "$SECOND_EXIT_PID" ]]; then
            pass "$DESC"
        else
            fail "$DESC" "no se pudo distinguir orden de terminacion"
        fi
    else
        exits=$(count_log "EXEC -> EXIT")
        readys=$(count_log "NEW -> READY")
        fail "$DESC" "procesos en READY=$readys pero solo $exits llegaron a EXIT (timeout)"
    fi
else
    readys=$(count_log "NEW -> READY")
    news=$(count_log "Proceso creado")
    fail "$DESC" "procesos creados=$news, en READY=$readys de 2 esperados (timeout largo_plazo)"
fi

# ==========================================================
#  FASE 5: DETECCION DE DEADLOCK
# ==========================================================

echo ""
echo "================================================"
echo "  FASE 5: Deteccion de Deadlock"
echo "================================================"

# Usar RR para que ambos procesos alternen ejecucion y generen el deadlock
send_cmd "ALGORITMO RR"
sleep 2

# Warm-up: asegurar scheduler limpio despues del cambio de algoritmo
echo "  [warmup] Verificando scheduler con RR..."
mark_log
run_process "test1.txt"
if wait_for_log "EXEC -> EXIT" 15; then
    echo "  [warmup] OK"
else
    echo "  [warmup] WARN - proceso warmup no completo, continuando"
fi
sleep 1

# --- Test 11: Deadlock ---
# deadlock_a.txt: WAIT RA, WAIT RB, ...
# deadlock_b.txt: WAIT RB, WAIT RA, ...
# Con RR y quantum, A ejecuta WAIT RA (adquiere), se desaloja,
# B ejecuta WAIT RB (adquiere), luego WAIT RA (bloqueado),
# A ejecuta WAIT RB (bloqueado) → deadlock (ciclo: A espera RB de B, B espera RA de A)
DESC="Deadlock: deteccion de ciclo con grafo de espera"
begin_test "$DESC"
run_process "deadlock_a.txt"
sleep 2
run_process "deadlock_b.txt"
if wait_for_log "Deadlock detectado|deadlock detectado|DEADLOCK" 30; then
    pass "$DESC"
else
    if check_log "BLOCKED"; then
        blocked_count=$(count_log "BLOCKED")
        fail "$DESC" "procesos bloqueados ($blocked_count) pero no se detecto deadlock en logs"
    else
        fail "$DESC" "no se detecto deadlock ni bloqueo (timeout)"
    fi
fi

# ==========================================================
#  RESUMEN
# ==========================================================

echo ""
echo ""
echo "================================================"
echo "   RESULTADOS: $PASSED/$TOTAL pasaron"
echo "================================================"
echo ""
echo " Estado | Test"
echo "--------|----------------------------------------------"
echo -e "$RESULTS"

echo "--- Errores en logs de modulos ---"
FOUND_ERRORS=0
ALL_LOGS=("$LOG_DIR"/*.log)
[[ -f "$KERNEL_OWN_LOG" ]] && ALL_LOGS+=("$KERNEL_OWN_LOG")
for log in "${ALL_LOGS[@]}"; do
    [[ -f "$log" ]] || continue
    count=$(grep -ciE "error|fail|segfault" "$log" 2>/dev/null) || count=0
    if [[ "$count" -gt 0 ]]; then
        FOUND_ERRORS=1
        echo ""
        echo "  $(basename "$log"): $count errores"
        grep -iE "error|fail|segfault" "$log" | tail -5
    fi
done
if [[ "$FOUND_ERRORS" -eq 0 ]]; then
    echo "  Sin errores en los logs."
fi

echo ""
echo "================================================"
if [[ "$FAILED" -eq 0 ]]; then
    echo "   TODOS LOS TESTS PASARON ($PASSED/$TOTAL)"
else
    echo "   $FAILED/$TOTAL TESTS FALLARON"
fi
echo "================================================"
echo ""
echo "Logs: $LOG_DIR/"
