#!/usr/bin/env bash
# Esegue sul PC i test degli algoritmi del pannello (sole, locator, indici, decoder CW/RTTY, QRZ).
set -euo pipefail
cd "$(dirname "$0")"
OUT="${TMPDIR:-/tmp}/loopant-test"
mkdir -p "$OUT"
g++ -std=c++17 -O2 -Wall -I../src test_sole.cpp ../src/sole.cpp ../src/locator.cpp -o "$OUT/test_sole"
g++ -std=c++17 -O2 -Wall -I../src test_indici.cpp ../src/indici.cpp -o "$OUT/test_indici"
g++ -std=c++17 -O2 -Wall -I../src test_decoder.cpp ../src/decoder/cw.cpp ../src/decoder/rtty.cpp -o "$OUT/test_decoder"
g++ -std=c++17 -O2 -Wall -I../src test_qrz.cpp ../src/qrz.cpp ../src/geo.cpp -o "$OUT/test_qrz"
"$OUT/test_sole" && "$OUT/test_indici" && "$OUT/test_decoder" && "$OUT/test_qrz"
