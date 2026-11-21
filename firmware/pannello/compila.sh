#!/usr/bin/env bash
# Compila (e, con "carica <porta>", carica) il firmware del pannello con arduino-cli.
# Requisiti: core esp32:esp32@3.1.1 e libreria lvgl@8.4.0 installati in arduino-cli.
#   ./compila.sh                      solo compilazione
#   ./compila.sh carica /dev/ttyACM0  compila e carica dalla porta "USB TO UART"
set -euo pipefail
cd "$(dirname "$0")"
QUI="$(pwd)"
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=default"
FLAGS="-DLV_CONF_INCLUDE_SIMPLE -I${QUI}"

arduino-cli compile --fqbn "$FQBN" \
  --library "${QUI}/../lib/Comune" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" \
  --build-property "compiler.c.extra_flags=${FLAGS}" \
  --warnings default \
  "$QUI"

if [[ "${1:-}" == "carica" ]]; then
  arduino-cli upload --fqbn "$FQBN" -p "${2:?porta mancante}" "$QUI"
fi
