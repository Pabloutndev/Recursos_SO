#!/usr/bin/env bash
# run_test.sh <script_name> [wait_secs]
# Corre un test individual enviando comandos a la consola remota via pipe.
#
# Ejemplo:
#   ./run_test.sh test1.txt
#   ./run_test.sh test_io.txt 8
#
# El script envia START + RUN <nombre> via la consola remota (modulo consola).
# START es idempotente: si ya se inicio la planificacion, no pasa nada.
#
# Exit codes:
#   0 = test paso (se detecto EXIT en kernel log)
#   1 = test fallo o no se pudo verificar

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_DIR="$SCRIPT_DIR/logs"
KERNEL_LOG="$LOG_DIR/kernel.log"

if [[ -z "$1" ]]; then
    echo "Uso: $0 <nombre_script> [segundos_espera]"
    echo ""
    echo "Scripts disponibles en memoria/procesos/:"
    for f in "$BASE_DIR/memoria/procesos/"*.txt; do
        echo "  $(basename "$f")"
    done
    exit 1
fi

SCRIPT_NAME="$1"
WAIT_TIME="${2:-5}"

# Verificar que el script existe en memoria/procesos/
if [[ ! -f "$BASE_DIR/memoria/procesos/$SCRIPT_NAME" ]]; then
    echo "ERROR: No se encontro '$SCRIPT_NAME' en memoria/procesos/"
    exit 1
fi

# Marcar linea actual del kernel log para despues buscar solo lineas nuevas
LINES_BEFORE=0
if [[ -f "$KERNEL_LOG" ]]; then
    LINES_BEFORE=$(wc -l < "$KERNEL_LOG")
fi

# Enviar comandos por pipe a la consola remota.
# readline() con stdin pipe lee linea por linea hasta EOF, despues la consola sale.
# START inicia la planificacion (idempotente), RUN envia el proceso al kernel.
(cd "$BASE_DIR/consola" && printf 'START\nRUN %s\n' "$SCRIPT_NAME" | ./bin/consola) > /dev/null 2>&1

sleep "$WAIT_TIME"

# Verificar resultado: buscar transicion a EXIT en las lineas nuevas del kernel log
if [[ -f "$KERNEL_LOG" ]]; then
    NUEVAS=$(tail -n +"$((LINES_BEFORE + 1))" "$KERNEL_LOG")
    if echo "$NUEVAS" | grep -qiE "EXIT|fin.proceso|finaliz"; then
        exit 0
    fi
fi

exit 1
