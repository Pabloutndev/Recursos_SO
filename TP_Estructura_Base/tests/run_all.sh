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
# 2. Ejecutar tests
# =============================
echo ""
echo "================================================"
echo "         Ejecutando tests"
echo "================================================"

run() {
    local name="$1"
    local wait="$2"
    local desc="$3"
    echo ""
    echo "--- [$desc] $name ---"
    "$SCRIPT_DIR/run_test.sh" "$name" "$wait"
}

run "test1.txt"     3  "Basico: SET/SUM/SUB/EXIT"
run "test_io.txt"   5  "IO simple: IO_GEN_SLEEP"
run "multi_io.txt"  8  "IO multiple: varios IO_GEN_SLEEP"
run "test_loop.txt" 5  "Loop: JNZ"
run "largo.txt"     12 "Loop largo: test quantum RR"

# =============================
# 3. Resultados
# =============================
echo ""
echo "================================================"
echo "         Resultados"
echo "================================================"
echo ""

# Transiciones de estado en el log del kernel
echo "--- Transiciones de estado (kernel.log) ---"
if [[ -f "$LOG_DIR/kernel.log" ]]; then
    grep -iE "estado|NEW|READY|EXEC|BLOCK|EXIT|quantum|desaloj|planif" \
        "$LOG_DIR/kernel.log" | tail -80 || echo "(sin coincidencias)"
else
    echo "(kernel.log no encontrado)"
fi

echo ""

# Resumen de errores en todos los logs
echo "--- Errores por modulo ---"
FOUND_ERRORS=0
for log in "$LOG_DIR"/*.log; do
    [[ -f "$log" ]] || continue
    count=$(grep -ciE "error|fail|segfault" "$log" 2>/dev/null) || count=0
    if [[ "$count" -gt 0 ]]; then
        FOUND_ERRORS=1
        echo ""
        echo "  $(basename "$log"): $count errores"
        grep -iE "error|fail|segfault" "$log" | tail -10
    fi
done

if [[ "$FOUND_ERRORS" -eq 0 ]]; then
    echo "  Sin errores detectados en los logs."
fi

# =============================
# 4. Fin (stop_modules.sh corre automaticamente via trap EXIT)
# =============================
echo ""
echo "================================================"
echo "         Tests completados"
echo "================================================"
echo "Logs disponibles en: $LOG_DIR/"
echo ""
echo "Para revisar manualmente:"
echo "  cat $LOG_DIR/kernel.log   | grep -i estado"
echo "  cat $LOG_DIR/memoria.log  | grep -i instruc"
echo "  cat $LOG_DIR/cpu.log      | grep -i exec"
