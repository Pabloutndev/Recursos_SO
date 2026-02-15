#!/usr/bin/env bash
# run_all.sh - Levanta modulos, corre la suite de tests, muestra resultados, y apaga todo.
#
# Uso:
#   cd tests && bash run_all.sh
#
# Tests ejecutados (usan los scripts existentes en memoria/procesos/):
#   1. test1.txt      - Basico: SET, SUM, SUB, EXIT
#   2. test_io.txt    - IO simple: IO_GEN_SLEEP una vez
#   3. multi_io.txt   - IO multiple: varios IO_GEN_SLEEP
#   4. test_loop.txt  - Loop con JNZ
#   5. largo.txt      - Loop largo, sirve para ver desalojo con RR
#
# Verificacion esperada en logs:
#   - Basico:    NEW -> READY -> EXEC -> EXIT
#   - IO:        EXEC -> BLOCKED -> READY -> EXEC -> EXIT
#   - Quantum:   Multiples EXEC -> READY por fin de quantum (con RR)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LOG_DIR="$SCRIPT_DIR/logs"

# Asegurar que los modulos se apaguen aunque el script falle a mitad
cleanup() {
    echo ""
    echo "=== Limpieza: deteniendo modulos ==="
    "$SCRIPT_DIR/stop_modules.sh" 2>/dev/null || true
}
trap cleanup EXIT

echo "================================================"
echo "         Test Suite - SO 2026"
echo "================================================"
echo ""

# =============================
# 1. Levantar modulos
# =============================
"$SCRIPT_DIR/start_modules.sh"

echo ""
echo "=== Esperando estabilizacion (3s) ==="
sleep 3

# =============================
# 2. Ejecutar tests y trackear resultados
# =============================
echo ""
echo "================================================"
echo "         Ejecutando tests"
echo "================================================"

TOTAL=0
PASSED=0
FAILED=0
RESULTS=""

run() {
    local name="$1"
    local wait="$2"
    local desc="$3"
    TOTAL=$((TOTAL + 1))

    echo ""
    echo "--- Test $TOTAL: $desc ---"
    echo "    Script: $name | Espera: ${wait}s"

    set +e
    "$SCRIPT_DIR/run_test.sh" "$name" "$wait"
    local rc=$?
    set -e

    if [[ $rc -eq 0 ]]; then
        PASSED=$((PASSED + 1))
        RESULTS="${RESULTS}  PASS  |  ${desc} (${name})\n"
        echo "    -> PASS"
    else
        FAILED=$((FAILED + 1))
        RESULTS="${RESULTS}  FAIL  |  ${desc} (${name})\n"
        echo "    -> FAIL (no se detecto EXIT en kernel log)"
    fi
}

run "test1.txt"     3  "Basico: SET/SUM/SUB/EXIT"
run "test_io.txt"   5  "IO simple: IO_GEN_SLEEP"
run "multi_io.txt"  8  "IO multiple: varios IO_GEN_SLEEP"
run "test_loop.txt" 5  "Loop: JNZ"
run "largo.txt"     12 "Loop largo: test quantum RR"

# =============================
# 3. Resumen de resultados
# =============================
echo ""
echo "================================================"
echo "         Resultados: $PASSED/$TOTAL pasaron"
echo "================================================"
echo ""
echo " Estado | Test"
echo "--------|--------------------------------------------"
echo -e "$RESULTS"

# Errores en logs
echo "--- Errores en logs ---"
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

# =============================
# 4. Fin (stop_modules.sh corre automaticamente via trap EXIT)
# =============================
echo ""
echo "================================================"
if [[ "$FAILED" -eq 0 ]]; then
    echo "  TODOS LOS TESTS PASARON ($PASSED/$TOTAL)"
else
    echo "  $FAILED/$TOTAL TESTS FALLARON"
fi
echo "================================================"
echo "Logs: $LOG_DIR/"
