#!/usr/bin/env bash
# Compila (e, con "carica <porta>", carica) il firmware dell'unità antenna con arduino-cli.
# Requisiti: core esp32:esp32@3.1.1 (lo stesso del pannello) e le librerie
#   FastAccelStepper@2.0.1, TMCStepper@0.7.3, U8g2 installate in arduino-cli.
#   ./compila.sh tx                       versione TX (NEMA11 + TMC2209)
#   ./compila.sh rx                       versione RX (SERVO42D, STEP/DIR o CAN)
#   ./compila.sh tx carica /dev/ttyUSB0   compila e carica
set -euo pipefail
cd "$(dirname "$0")"
QUI="$(pwd)"
case "${1:-}" in
  tx) TX=1 ;;
  rx) TX=0 ;;
  *) echo "uso: $0 tx|rx [carica <porta>]"; exit 1 ;;
esac
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
FLAGS="-I${QUI}/include -DVERSIONE_TX=${TX}"
USCITA="${TMPDIR:-/tmp}/loopant-antenna-$1"

arduino-cli compile --fqbn "$FQBN" \
  --library "${QUI}/../lib/Comune" \
  --build-path "$USCITA" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" \
  --build-property "compiler.c.extra_flags=${FLAGS}" \
  --warnings default \
  "$QUI"

if [[ "${2:-}" == "carica" ]]; then
  arduino-cli upload --fqbn "$FQBN" --input-dir "$USCITA" -p "${3:?porta mancante}" "$QUI"
fi
