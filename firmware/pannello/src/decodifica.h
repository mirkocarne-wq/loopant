// Decoder CW/RTTY: sul pannello (ingresso J8) oppure, se la manopola remota è associata, quello della manopola
// (testo e stato arrivano da link_manopola). Configurazione dalle impostazioni, testo, stato per l'interfaccia.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace decodifica {

struct Stato {
  bool attivo;          // decoder acceso (spento: niente elaborazione, né qui né sulla manopola)
  bool remoto;          // decoder della manopola remota
  bool remotoCollegato; // manopola collegata
  uint8_t sorgente;     // proto::SorgenteAudio della manopola
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
// Decoder acceso o spento: stato unico per pannello e manopola, salvato in NVS ("decoder"). Si cambia dalla
// pagina Decoder o dal menu della manopola (attraverso link_manopola).
bool attivo();
void attiva(bool si);
void aggiungiRemoto(const char* testo, size_t n);  // caratteri decodificati dalla manopola (da link_manopola)

}  // namespace decodifica
