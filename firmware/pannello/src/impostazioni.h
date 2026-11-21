// Impostazioni del pannello salvate in NVS.
#pragma once
#include <stdint.h>

struct Impostazioni {
  uint8_t versione;
  char nominativo[11];
  char locator[7];
  uint8_t tema;     // indice in tema::ELENCO
  uint8_t peer[6];  // MAC dell'unità antenna
  uint32_t ultima40_hz;
  uint32_t ultima20_hz;

  // Schermo e ora
  uint8_t screensaver;  // 0 orologio, 1 frequenza, 2 grey line, 3 spento
  uint8_t attesa_min;   // 0 = mai
  uint8_t luminosita;   // 10-100 %
  uint8_t modoOra;      // 0 UTC, 1 locale, 2 entrambe
  uint8_t fuso;         // 0 UTC, 1 Europa centrale (CET/CEST)

  // Rete (indici solari e NTP)
  char wifiSsid[33];
  char wifiPassword[65];

  // Decoder
  uint8_t decModo;      // 0 CW, 1 RTTY
  uint16_t cwTono;      // Hz
  uint16_t cwFiltro;    // Hz: 100, 250, 500
  uint8_t cwWpm;        // 0 = automatico
  uint8_t rttyBaud;     // 0 = 45,45, 1 = 50, 2 = 75
  uint16_t rttyShift;   // Hz: 170, 425, 850
  uint16_t rttyMark;    // Hz
  uint8_t rttyStop;     // 0 = 1, 1 = 1,5, 2 = 2
  uint8_t rttyOpzioni;  // bit 0 Reverse, bit 1 UOS, bit 2 AFC
};

namespace impostazioni {

constexpr uint8_t RTTY_REVERSE = 1 << 0;
constexpr uint8_t RTTY_UOS = 1 << 1;
constexpr uint8_t RTTY_AFC = 1 << 2;

void carica();
void salva();
Impostazioni& get();

}  // namespace impostazioni
