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

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

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

echo "=== Test: $SCRIPT_NAME ==="

# Enviar comandos por pipe a la consola remota.
# readline() con stdin pipe lee linea por linea hasta EOF, despues la consola sale.
# START inicia la planificacion (idempotente), RUN envia el proceso al kernel.
(cd "$BASE_DIR/consola" && printf 'START\nRUN %s\n' "$SCRIPT_NAME" | ./bin/consola) 2>&1

echo "  Enviado. Esperando ${WAIT_TIME}s para que el proceso termine..."
sleep "$WAIT_TIME"
echo "  Listo."
