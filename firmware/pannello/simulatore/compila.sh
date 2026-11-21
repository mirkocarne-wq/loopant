#!/usr/bin/env bash
# Compila il simulatore sul PC (Linux, gcc) e salva le schermate del pannello in PNG.
# Uso: compila.sh <cartella della libreria lvgl 8.4> [cartella di uscita]
# La cartella lvgl è quella installata da arduino-cli (es. ~/Arduino/libraries/lvgl).
set -euo pipefail
LVGL="$(cd "${1:?cartella lvgl mancante}" && pwd)"
QUI="$(cd "$(dirname "$0")" && pwd)"
PANNELLO="$QUI/.."
USCITA="${2:-$QUI/schermate}"
COSTRUZIONE="${TMPDIR:-/tmp}/loopant-simulatore"
mkdir -p "$COSTRUZIONE/lvgl" "$COSTRUZIONE/font" "$USCITA"

FLAGS="-O1 -DLV_CONF_INCLUDE_SIMPLE -DIRAM_ATTR= -I$PANNELLO -I$LVGL -w"
# LVGL (compilato una volta sola)
if [[ ! -f "$COSTRUZIONE/lvgl/.fatto" ]]; then
  find "$LVGL/src" -name '*.c' | while read -r f; do
    gcc $FLAGS -c "$f" -o "$COSTRUZIONE/lvgl/$(echo "$f" | md5sum | cut -c1-12).o"
  done
  touch "$COSTRUZIONE/lvgl/.fatto"
fi
for f in "$PANNELLO"/src/font/*.c; do gcc $FLAGS -c "$f" -o "$COSTRUZIONE/font/$(basename "$f" .c).o"; done

SORGENTI=(
  "$QUI/simulatore.cpp"
  "$PANNELLO"/src/ui/*.cpp
  "$PANNELLO"/src/{display7,tema,mappa,sole,locator,eventi,memorie,impostazioni,ora,indici}.cpp
)
g++ -std=c++17 -O1 -DLV_CONF_INCLUDE_SIMPLE -DIRAM_ATTR= -I"$QUI/stub" -I"$PANNELLO/src" -I"$PANNELLO" \
  -I"$PANNELLO/../lib/Comune/src" -I"$LVGL" "${SORGENTI[@]}" \
  "$COSTRUZIONE"/lvgl/*.o "$COSTRUZIONE"/font/*.o -lm -o "$COSTRUZIONE/simulatore"

"$COSTRUZIONE/simulatore" "$USCITA" >/dev/null
python3 - "$USCITA" <<'EOF'
import sys, glob, os
from PIL import Image
for p in sorted(glob.glob(os.path.join(sys.argv[1], '*.ppm'))):
    Image.open(p).save(p[:-4] + '.png')
    os.remove(p)
    print(p[:-4] + '.png')
EOF
