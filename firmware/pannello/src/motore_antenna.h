// Tipo di antenna (firmware unico), scelto dal pannello: TX accorda in STEP/DIR (TMC2209), RX via CAN (SERVO42D).
// Per l'RX anche indirizzo e velocità del bus CAN.
// Le scelte si salvano qui (NVS "motore") e si inviano all'antenna con il comando IMPOSTA.
#pragma once
#include <stdint.h>

namespace motore_antenna {

struct Scelte {
  uint8_t tipo;      // 0 RX, 1 TX (segue quello dichiarato dall'antenna)
  uint16_t canId;    // 1-2047
  uint8_t kbitIdx;   // 0 = 125, 1 = 250, 2 = 500, 3 = 1000
};

void carica();
Scelte& scelte();
uint16_t kbit();  // dalla scelta
void inviaTipo();  // l'antenna risponde e si riavvia con il tipo nuovo
void inviaCan();  // indirizzo e velocità

}  // namespace motore_antenna
