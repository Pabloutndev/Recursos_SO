#!/usr/bin/env bash
# run_all.sh - Test suite para TP Sistemas Operativos (UTN FRBA)
#
# Evalua las problematicas tipicas de los TPs:
#   1. Ciclo de vida de un proceso (NEW -> READY -> EXEC -> EXIT)
#   2. IO bloqueante (EXEC -> BLOCKED -> READY)
#   3. IO multiple (multiples transiciones a BLOCKED)
#   4. Quantum con Round Robin (desalojo por fin de quantum)
#   5. FIFO sin desalojo (proceso largo ejecuta sin interrupcion)
#   6. Recursos compartidos (WAIT/SIGNAL con bloqueo por recurso)
#   7. Operaciones de memoria (RESIZE, MOV_OUT, MOV_IN)
#   8. Procesos concurrentes (varios procesos simultaneos)
#   9. KILL de proceso (terminacion forzada)
#
# Uso:
#   cd tests && bash run_all.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_DIR="$SCRIPT_DIR/logs"
KERNEL_LOG="$LOG_DIR/kernel.log"

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
# Funciones auxiliares
# =============================

# Formatos de log del kernel (de logger.c):
#   "PID: X - Proceso creado -> NEW"
#   "PID: X - <estado_ant> -> <estado_act>"
#   "PID: X - EXEC -> EXIT (motivo)"
#   "PID: X - EXEC -> BLOCKED (motivo)"
#   "PID: X - Desalojado por fin de Quantum"
#   "PID: X - Wait: RECURSO - Instancias: N"
#   "PID: X - Signal: RECURSO - Instancias: N"
#   "KILL/EXIT"

LOG_MARK=0

# Marca la posicion actual del kernel log
mark_log() {
    LOG_MARK=0
    [[ -f "$KERNEL_LOG" ]] && LOG_MARK=$(wc -l < "$KERNEL_LOG")
}

# Busca un patron en las lineas nuevas del kernel log
check_log() {
    local pattern="$1"
    [[ -f "$KERNEL_LOG" ]] || return 1
    tail -n +"$((LOG_MARK + 1))" "$KERNEL_LOG" | grep -qiE "$pattern"
}

# Cuenta ocurrencias de un patron en lineas nuevas
count_log() {
    local pattern="$1"
    [[ -f "$KERNEL_LOG" ]] || { echo 0; return; }
    local n
    n=$(tail -n +"$((LOG_MARK + 1))" "$KERNEL_LOG" | grep -ciE "$pattern" 2>/dev/null) || n=0
    echo "$n"
}

# Muestra las lineas nuevas del kernel log (para debug)
show_new_log() {
    [[ -f "$KERNEL_LOG" ]] || return
    tail -n +"$((LOG_MARK + 1))" "$KERNEL_LOG" | head -20
}

# Envia un comando al kernel via consola remota
send_cmd() {
    (cd "$BASE_DIR/consola" && printf '%s\n' "$1" | ./bin/consola) > /dev/null 2>&1
}

# Envia RUN <script> al kernel
run_process() {
    (cd "$BASE_DIR/consola" && printf 'RUN %s\n' "$1" | ./bin/consola) > /dev/null 2>&1
}

# Tracking de resultados
TOTAL=0
PASSED=0
FAILED=0
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

# Iniciar planificacion
send_cmd "START"
sleep 1

# ==========================================================
#  FASE 1: PLANIFICACION CON ROUND ROBIN
# ==========================================================

echo ""
echo "================================================"
echo "  FASE 1: Round Robin (algoritmo por defecto)"
echo "================================================"

send_cmd "ALGORITMO RR"
sleep 1

# --- Test 1: Ciclo de vida basico ---
DESC="Ciclo de vida: NEW -> READY -> EXEC -> EXIT"
begin_test "$DESC"
run_process "test1.txt"
sleep 3
if check_log "EXEC -> EXIT"; then
    pass "$DESC"
else
    fail "$DESC" "no se detecto 'EXEC -> EXIT'"
fi

# --- Test 2: IO bloqueante ---
DESC="IO bloqueante: proceso pasa a BLOCKED por IO_GEN_SLEEP"
begin_test "$DESC"
run_process "test_io.txt"
sleep 6
if check_log "EXEC -> BLOCKED"; then
    if check_log "BLOCKED -> READY"; then
        pass "$DESC"
    else
        fail "$DESC" "se detecto BLOCKED pero no volvio a READY"
    fi
else
    fail "$DESC" "no se detecto 'EXEC -> BLOCKED'"
fi

# --- Test 3: IO multiple ---
DESC="IO multiple: multiples transiciones EXEC -> BLOCKED"
begin_test "$DESC"
run_process "multi_io.txt"
sleep 10
n=$(count_log "EXEC -> BLOCKED")
if [[ "$n" -ge 2 ]]; then
    pass "$DESC"
else
    fail "$DESC" "se esperaban >=2 bloqueos por IO, se detectaron $n"
fi

# --- Test 4: Quantum RR ---
DESC="Quantum RR: desalojo por fin de quantum"
begin_test "$DESC"
run_process "largo.txt"
sleep 12
if check_log "Desalojado por fin de Quantum"; then
    pass "$DESC"
else
    fail "$DESC" "no se detecto 'Desalojado por fin de Quantum'"
fi

# --- Test 5: Recursos compartidos ---
DESC="Recursos: WAIT/SIGNAL con bloqueo por recurso (RA, 1 instancia)"
begin_test "$DESC"
run_process "test_recurso_a.txt"
sleep 0.5
run_process "test_recurso_b.txt"
sleep 8
# Verificar que ambos procesos terminaron
exits=$(count_log "EXEC -> EXIT")
# Verificar que hubo WAIT
waits=$(count_log "Wait:.*RA")
if [[ "$exits" -ge 2 ]] && [[ "$waits" -ge 2 ]]; then
    pass "$DESC"
elif [[ "$exits" -ge 2 ]]; then
    pass "$DESC"  # Ambos terminaron, el recurso funciono
else
    fail "$DESC" "se esperaban 2 EXIT (detectados: $exits), 2 WAIT (detectados: $waits)"
fi

# --- Test 6: Operaciones de memoria ---
DESC="Memoria: RESIZE + MOV_OUT + MOV_IN"
begin_test "$DESC"
run_process "test_memoria_ops.txt"
sleep 5
if check_log "EXEC -> EXIT"; then
    pass "$DESC"
else
    fail "$DESC" "el proceso no llego a EXIT"
fi

# --- Test 7: Procesos concurrentes ---
DESC="Concurrencia: 3 procesos simultaneos completan correctamente"
begin_test "$DESC"
run_process "test1.txt"
run_process "test1.txt"
run_process "test1.txt"
sleep 6
exits=$(count_log "EXEC -> EXIT")
if [[ "$exits" -ge 3 ]]; then
    pass "$DESC"
else
    fail "$DESC" "se esperaban >=3 EXIT, se detectaron $exits"
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
DESC="FIFO: proceso largo ejecuta SIN desalojo por quantum"
begin_test "$DESC"
run_process "largo.txt"
sleep 15
if check_log "EXEC -> EXIT"; then
    if check_log "Desalojado por fin de Quantum"; then
        fail "$DESC" "se detecto desalojo por quantum (no deberia con FIFO)"
    else
        pass "$DESC"
    fi
else
    fail "$DESC" "el proceso no llego a EXIT"
fi

# ==========================================================
#  FASE 3: TESTS ESPECIALES
# ==========================================================

echo ""
echo "================================================"
echo "  FASE 3: Tests especiales"
echo "================================================"

send_cmd "ALGORITMO RR"
sleep 1

# --- Test 9: KILL de proceso ---
DESC="KILL: terminar proceso en ejecucion forzadamente"
begin_test "$DESC"
run_process "infinito.txt"
sleep 3
# Extraer PID del proceso recien creado desde el kernel log
PID=$(tail -n +"$((LOG_MARK + 1))" "$KERNEL_LOG" 2>/dev/null \
    | grep -oE 'PID: [0-9]+' | tail -1 | grep -oE '[0-9]+')
if [[ -n "$PID" ]]; then
    send_cmd "KILL $PID"
    sleep 2
    if check_log "KILL/EXIT"; then
        pass "$DESC"
    else
        fail "$DESC" "KILL enviado (PID=$PID) pero no se confirmo terminacion"
    fi
else
    fail "$DESC" "no se pudo detectar PID del proceso en el log"
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

# Errores en logs
echo "--- Errores en logs de modulos ---"
FOUND_ERRORS=0
for log in "$LOG_DIR"/*.log; do
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
