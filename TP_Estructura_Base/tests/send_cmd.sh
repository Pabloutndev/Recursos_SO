#!/usr/bin/env bash
# send_cmd.sh <comando>
# Envia un comando al kernel via la consola remota.
#
# Ejemplos:
#   ./send_cmd.sh "START"
#   ./send_cmd.sh "ALGORITMO FIFO"
#   ./send_cmd.sh "KILL 3"
#   ./send_cmd.sh "PS"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ -z "$1" ]]; then
    echo "Uso: $0 <comando>"
    echo "Comandos: START, PAUSE, RUN <script>, KILL <pid>, PS, ALGORITMO <alg>, DESALOJAR <pid>"
    exit 1
fi

(cd "$BASE_DIR/consola" && printf '%s\n' "$1" | ./bin/consola) 2>&1
