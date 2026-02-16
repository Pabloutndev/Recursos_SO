#!/usr/bin/env bash
# start_modules.sh - Compila y levanta todos los modulos del TP en background.
# Cada modulo se lanza desde su propia carpeta (los configs usan paths relativos).
# El kernel tiene readline en stdin: se usa tail -f /dev/null para que no reciba EOF.
# Se usa stdbuf -oL para forzar line-buffered en stdout (asi los logs se ven en tiempo real).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

LOG_DIR="$SCRIPT_DIR/logs"
PID_FILE="$SCRIPT_DIR/.pids"

mkdir -p "$LOG_DIR"

# =============================
# Limpieza de corridas anteriores
# =============================

# Intentar stop limpio si hay PIDs de antes
if [[ -s "$PID_FILE" ]]; then
    "$SCRIPT_DIR/stop_modules.sh" 2>/dev/null || true
fi

# Matar por nombre de binario
for proc in memoria kernel cpu entradasalida consola; do
    pkill -f "bin/$proc" 2>/dev/null || true
done
pkill -f "tail -f /dev/null" 2>/dev/null || true

# Liberar puertos especificos (8002=memoria, 8003=kernel, 8005=kernel_io,
# 8006=cpu_dispatch, 8007=cpu_interrupt, 8010=consola)
PUERTOS="8002 8003 8005 8006 8007 8010"
for port in $PUERTOS; do
    fuser -k "$port/tcp" 2>/dev/null || true
done

sleep 2

# Esperar hasta que los puertos esten libres (max 15 segundos)
echo -n "  Esperando puertos libres..."
for i in $(seq 1 15); do
    ALL_FREE=true
    for port in $PUERTOS; do
        if fuser "$port/tcp" >/dev/null 2>&1; then
            ALL_FREE=false
            break
        fi
    done
    if $ALL_FREE; then
        echo " OK"
        break
    fi
    echo -n "."
    sleep 1
done
if ! $ALL_FREE; then
    echo " WARN: algunos puertos siguen ocupados, continuando igual"
fi
> "$PID_FILE"

# Limpiar logs anteriores del kernel (se crean en su CWD)
rm -f "$BASE_DIR/kernel/kernel.log" "$BASE_DIR/kernel/kernel_error.log"

# =============================
# Compilacion
# =============================
echo "=== Compilando modulos ==="

make -C "$BASE_DIR/utils" > /dev/null 2>&1
echo "  [OK] utils"

for mod in memoria kernel cpu entradasalida consola; do
    make -C "$BASE_DIR/$mod" > /dev/null 2>&1
    echo "  [OK] $mod"
done

echo ""

# =============================
# Levantar modulos en orden
# =============================
echo "=== Levantando modulos ==="

# 1) Memoria - servidor, no usa readline, stdin desde /dev/null
echo -n "  [1/4] Memoria... "
(cd "$BASE_DIR/memoria" && stdbuf -oL ./bin/memoria) < /dev/null > "$LOG_DIR/memoria.log" 2>&1 &
echo $! >> "$PID_FILE"
sleep 2
echo "PID $!"

# 2) CPU - cliente de memoria + servidor para kernel, no usa readline
echo -n "  [2/4] CPU... "
(cd "$BASE_DIR/cpu" && stdbuf -oL ./bin/cpu) < /dev/null > "$LOG_DIR/cpu.log" 2>&1 &
echo $! >> "$PID_FILE"
sleep 1
echo "PID $!"

# 3) Kernel - tiene readline en su consola local.
#    Usamos tail -f /dev/null para mantener stdin abierto sin enviar datos,
#    asi readline() bloquea en vez de recibir EOF.
echo -n "  [3/4] Kernel... "
tail -f /dev/null | (cd "$BASE_DIR/kernel" && stdbuf -oL ./bin/kernel) > "$LOG_DIR/kernel.log" 2>&1 &
KERNEL_PID=$!
echo $! >> "$PID_FILE"
sleep 2
echo "PID $KERNEL_PID"

# 4) IO Generica (GENERICA1)
echo -n "  [4/4] IO GENERICA1... "
(cd "$BASE_DIR/entradasalida" && stdbuf -oL ./bin/entradasalida GENERICA1 generica.config) < /dev/null > "$LOG_DIR/io_generica.log" 2>&1 &
echo $! >> "$PID_FILE"
sleep 1
echo "PID $!"

echo ""
echo "=== Todos los modulos levantados ==="
echo "PIDs: $(tr '\n' ' ' < "$PID_FILE")"
echo "Logs: $LOG_DIR/"
