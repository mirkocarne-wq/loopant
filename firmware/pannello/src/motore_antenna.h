// Driver del motore dell'antenna RX (STEP/DIR o CAN con il SERVO42D), scelto dal pannello.
// Le scelte si salvano qui (NVS "motore") e si inviano all'antenna con il comando IMPOSTA.
#pragma once
#include <stdint.h>

namespace motore_antenna {

struct Scelte {
  uint8_t modo;      // 0 STEP/DIR, 1 CAN
  uint16_t canId;    // 1-2047
  uint8_t kbitIdx;   // 0 = 125, 1 = 250, 2 = 500, 3 = 1000
};

void carica();
Scelte& scelte();
uint16_t kbit();  // dalla scelta
void inviaModo();
void inviaCan();  // indirizzo e velocità

}  // namespace motore_antenna
