// Impostazioni del pannello salvate in NVS.
#pragma once
#include <stdint.h>

struct Impostazioni {
  uint8_t versione;
  char nominativo[11];
  char locator[7];
  uint8_t tema;  // indice in tema::ELENCO
  uint8_t peer[6];  // MAC dell'unità antenna
  uint32_t ultima40_hz;
  uint32_t ultima20_hz;
  uint8_t screensaver;  // 0 orologio, 1 frequenza, 2 grey line, 3 spento
  uint8_t attesa_min;   // 0 = mai
  uint8_t luminosita;   // 10-100 %
};

namespace impostazioni {

void carica();
void salva();
Impostazioni& get();

}  // namespace impostazioni
