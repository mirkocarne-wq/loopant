// Decoder CW/RTTY sul pannello: configurazione dalle impostazioni, testo ricevuto, stato per l'interfaccia.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace decodifica {

struct Stato {
  bool ingresso;        // ingresso audio attivo
  float audio;          // livello audio 0-1
  bool rtty;            // modo attuale
  // CW
  float cwLivello, cwSoglia, cwWpm;
  bool cwTasto;
  char cwElementi[8];
  // RTTY
  float rttyMark, rttySpace, rttyAfc;
  bool rttyAgganciato;
};

void inizia();      // avvia l'ingresso audio
void configura();   // rilegge le impostazioni (modo e parametri)
Stato stato();
uint32_t testo(char* out, size_t n);  // copia il testo ricevuto; restituisce un contatore che cambia a ogni carattere
void pulisci();

}  // namespace decodifica
