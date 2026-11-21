// Collegamento ESP-NOW con l'unità antenna: comandi, heartbeat, ripetizioni, stato.
// aggiorna() va chiamata dal loop(); invia() e stato() si possono chiamare anche dal task di LVGL.
#pragma once
#include <protocollo.h>
#include <stdint.h>

namespace link_antenna {

struct Stato {
  bool collegato;        // risposte ricevute negli ultimi TIMEOUT_COLLEGAMENTO_MS
  bool ricevuta;         // almeno una risposta dall'accensione
  bool peerImpostato;    // MAC dell'antenna impostato
  bool comandoFallito;   // l'ultimo comando non ha avuto risposta dopo MAX_TENTATIVI
  proto::Risposta r;     // ultima risposta
  uint32_t etaMs;        // tempo dall'ultima risposta
  uint32_t persi;        // comandi ripetuti
  uint32_t inviati;
};

void inizia(const uint8_t peer[6]);
void aggiorna();
void invia(proto::Tipo tipo, uint32_t freq_hz = 0, int32_t valore = 0);
Stato stato();

}  // namespace link_antenna
