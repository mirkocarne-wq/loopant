#!/usr/bin/env bash
# Compila (e, con "carica <porta>", carica) il firmware del pannello con arduino-cli.
# Schede: Waveshare ESP32-S3-Touch-LCD-7B (predefinita) o ESP32-S3-Touch-LCD-5B (5", 1024x600): primo argomento "5b".
# Requisiti: core esp32:esp32@3.1.1 e libreria lvgl@8.4.0 installati in arduino-cli.
#   ./compila.sh                          7B, solo compilazione
#   ./compila.sh carica /dev/ttyACM0      7B, compila e carica (va bene una qualsiasi delle due USB-C)
#   ./compila.sh 5b carica /dev/ttyACM0   5B
set -euo pipefail
cd "$(dirname "$0")"
QUI="$(pwd)"
SCHEDA=7b
if [[ "${1:-}" == "5b" || "${1:-}" == "7b" ]]; then
  SCHEDA="$1"
  shift
fi
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=cdc"
FLAGS="-DLV_CONF_INCLUDE_SIMPLE -I${QUI}"
[[ "$SCHEDA" == "5b" ]] && FLAGS="$FLAGS -DPANNELLO_5B=1"
USCITA="${TMPDIR:-/tmp}/loopant-pannello-$SCHEDA"

arduino-cli compile --fqbn "$FQBN" \
  --library "${QUI}/../lib/Comune" \
  --library "${QUI}/../lib/Decoder" \
  --library "${QUI}/../lib/Grafica" \
  --build-path "$USCITA" \
  --build-property "compiler.cpp.extra_flags=${FLAGS}" \
  --build-property "compiler.c.extra_flags=${FLAGS}" \
  --warnings default \
  "$QUI"

if [[ "${1:-}" == "carica" ]]; then
  arduino-cli upload --fqbn "$FQBN" --input-dir "$USCITA" -p "${2:?porta mancante}" "$QUI"
fi
