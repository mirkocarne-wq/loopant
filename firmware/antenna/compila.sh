#!/usr/bin/env bash
# Compila (e, con "carica <porta>", carica) il firmware dell'unità antenna con arduino-cli.
# Firmware unico per TX e RX: il tipo di antenna si sceglie dal pannello (Impostazioni -> Antenna)
# o dalla console ("tipo tx|rx") e resta in NVS.
# Requisiti: core esp32:esp32@3.1.1 (lo stesso del pannello) e le librerie
#   FastAccelStepper@2.0.1, TMCStepper@0.7.3, U8g2 installate in arduino-cli.
#   ./compila.sh                       solo compilazione
#   ./compila.sh carica /dev/ttyUSB0   compila e carica
set -euo pipefail
cd "$(dirname "$0")"
QUI="$(pwd)"
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
FLAGS="-I${QUI}/include"
USCITA="${TMPDIR:-/tmp}/loopant-antenna"

arduino-cli compile --fqbn "$FQBN" \
  --library "${QUI}/../lib/Comune" \
  --build-path "$USCITA" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" \
  --build-property "compiler.c.extra_flags=${FLAGS}" \
  --warnings default \
  "$QUI"

if [[ "${1:-}" == "carica" ]]; then
  arduino-cli upload --fqbn "$FQBN" --input-dir "$USCITA" -p "${2:?porta mancante}" "$QUI"
fi
