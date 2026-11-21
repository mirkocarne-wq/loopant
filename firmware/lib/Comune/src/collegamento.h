// Collegamento ESP-NOW cifrato con un solo peer.
// I pacchetti ricevuti finiscono in una coda e si leggono dal loop con ricevi().
#pragma once
#include <Arduino.h>

struct Pacchetto {
  uint8_t len;
  int8_t rssi;  // dBm; 0 se non disponibile (core Arduino 2.x)
  uint8_t dati[64];
};

class Collegamento {
 public:
  // peer: MAC dell'altra unità; pmk/lmk: chiavi da 16 byte; canale: canale WiFi comune.
  bool inizia(const uint8_t peer[6], const uint8_t pmk[16], const uint8_t lmk[16], uint8_t canale);
  bool invia(const void* dati, size_t len);
  bool ricevi(Pacchetto& p);
  bool peerValido() const;

  static void macLocale(uint8_t out[6]);
  static bool leggiMac(const char* testo, uint8_t out[6]);
  static void scriviMac(const uint8_t mac[6], char out[18]);
};
