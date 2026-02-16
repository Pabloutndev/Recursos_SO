#!/usr/bin/env bash
# stop_modules.sh - Mata todos los modulos levantados por start_modules.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PID_FILE="$SCRIPT_DIR/.pids"

if [[ ! -f "$PID_FILE" ]]; then
    echo "No se encontro archivo de PIDs ($PID_FILE)."
    echo "Ejecuta start_modules.sh primero."
    exit 1
fi

echo "=== Deteniendo modulos ==="

# Primera pasada: SIGTERM
while read -r pid; do
    [[ -z "$pid" ]] && continue
    if kill -0 "$pid" 2>/dev/null; then
        echo "  Enviando SIGTERM a PID $pid..."
        kill "$pid" 2>/dev/null || true
    else
        echo "  PID $pid ya no existe"
    fi
done < "$PID_FILE"

sleep 2

# Segunda pasada: SIGKILL para los que sobrevivieron
while read -r pid; do
    [[ -z "$pid" ]] && continue
    if kill -0 "$pid" 2>/dev/null; then
        echo "  Forzando SIGKILL a PID $pid..."
        kill -9 "$pid" 2>/dev/null || true
    fi
done < "$PID_FILE"

# Matar posibles procesos huerfanos (tail, subshells)
for proc in memoria kernel cpu entradasalida consola; do
    pkill -f "bin/$proc" 2>/dev/null || true
done
pkill -f "tail -f /dev/null" 2>/dev/null || true

rm -f "$PID_FILE"

echo "=== Modulos detenidos ==="

# Opcion: limpiar logs
if [[ "$1" == "--clean-logs" ]]; then
    echo "Limpiando logs..."
    rm -rf "$SCRIPT_DIR/logs"
fi
