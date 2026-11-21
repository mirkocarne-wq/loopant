#!/usr/bin/env bash
# Compila (e, con "carica <porta>", carica) il firmware della manopola remota con arduino-cli.
# Scheda: Waveshare ESP32-S3-Knob-Touch-LCD-1.8 (ESP32-S3R8, 16 MB di flash, USB nativa).
# Requisiti: core esp32:esp32@3.1.1 e lvgl@8.4.0 installati in arduino-cli (come per il pannello).
#   ./compila.sh                       solo compilazione
#   ./compila.sh carica /dev/cu.usbmodem101
set -euo pipefail
cd "$(dirname "$0")"
QUI="$(pwd)"
FQBN="esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=cdc"
FLAGS="-DLV_CONF_INCLUDE_SIMPLE -I${QUI}"
USCITA="${TMPDIR:-/tmp}/loopant-manopola"

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
