#!/usr/bin/env bash
# Genera i font del pannello (Montserrat Medium + simboli LVGL) con le lettere accentate.
# Uso: genera_font.sh <cartella con Montserrat-Medium.ttf e FontAwesome5-Solid+Brands+Regular.woff>
#   (sono nella libreria lvgl 8.4: lvgl/scripts/built_in_font/)
# Richiede lv_font_conv 1.5.3 (npm install lv_font_conv@1.5.3) nel PATH o in LV_FONT_CONV.
set -euo pipefail
SORGENTI="${1:?cartella dei font mancante}"
CONV="${LV_FONT_CONV:-lv_font_conv}"
cd "$(dirname "$0")/../src/font"

# ASCII, Latin-1 (à è é ì ò ù · ° ×), – — ’ … − → ↔ •
CARATTERI="0x20-0x7E,0xA0-0xFF,0x2013,0x2014,0x2019,0x2026,0x2212,0x2192,0x2194,0x2022"
# Gli stessi simboli dei font integrati in LVGL (LV_SYMBOL_*)
SIMBOLI="61441,61448,61451,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,62099,62212,62189,62810,63426,63650"

for DIM in 12 14 16 18 20 22 24 26 28 30 34; do
  "$CONV" --bpp 4 --size "$DIM" --no-compress --format lvgl --force-fast-kern-format \
    --font "$SORGENTI/Montserrat-Medium.ttf" -r "$CARATTERI" \
    --font "$SORGENTI/FontAwesome5-Solid+Brands+Regular.woff" -r "$SIMBOLI" \
    --lv-include lvgl.h -o "font_${DIM}.c"
  # in Arduino la libreria si include come <lvgl.h>
  sed -i 's|#include "lvgl/lvgl.h"|#include <lvgl.h>|; s|#include "lvgl.h"|#include <lvgl.h>|' "font_${DIM}.c"
  # niente percorsi locali nell'intestazione
  sed -i "s|${SORGENTI%/}/||g" "font_${DIM}.c"
  echo "font_${DIM}.c"
done
