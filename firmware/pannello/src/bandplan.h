// Band plan IARU Regione 1 semplificato, dagli 80 ai 10 m (da verificare sul band plan ufficiale e sulle norme
// italiane, soprattutto i 60 m). Frequenze FT8 comuni come indicazione in più.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace bandplan {

struct Segmento {
  uint32_t da_khz10, a_khz10;  // decimi di kHz, estremo superiore escluso (l'ultimo della banda incluso)
  const char* nome;
};

// In ordine di frequenza
constexpr Segmento SEGMENTI[] = {
    {35000, 35700, "CW"},
    {35700, 36000, "Digitale a banda stretta"},
    {36000, 38000, "Tutti i modi"},
    {53515, 53540, "CW e digitale a banda stretta"},
    {53540, 53660, "Tutti i modi"},
    {53660, 53665, "Segnali deboli"},
    {70000, 70400, "CW"},
    {70400, 70500, "Digitale a banda stretta"},
    {70500, 72000, "Tutti i modi"},
    {101000, 101300, "CW"},
    {101300, 101500, "Digitale a banda stretta"},
    {140000, 140700, "CW"},
    {140700, 140990, "Digitale a banda stretta"},
    {140990, 141010, "Radiofari"},
    {141010, 143500, "Tutti i modi"},
    {180680, 180950, "CW"},
    {180950, 181090, "Digitale a banda stretta"},
    {181090, 181110, "Radiofari"},
    {181110, 181680, "Tutti i modi"},
    {210000, 210700, "CW"},
    {210700, 211490, "Digitale a banda stretta"},
    {211490, 211510, "Radiofari"},
    {211510, 214500, "Tutti i modi"},
    {248900, 249150, "CW"},
    {249150, 249290, "Digitale a banda stretta"},
    {249290, 249310, "Radiofari"},
    {249310, 249900, "Tutti i modi"},
    {280000, 280700, "CW"},
    {280700, 281900, "Digitale a banda stretta"},
    {281900, 282250, "Radiofari"},
    {282250, 293000, "Tutti i modi"},
    {293000, 295100, "Satelliti"},
    {295100, 295200, "Banda di guardia"},
    {295200, 297000, "FM"},
};

// Frequenze FT8 (kHz, finestra di 3 kHz sopra la frequenza indicata e 2 sotto)
constexpr uint32_t FT8_KHZ[] = {3573, 5357, 7074, 10136, 14074, 18100, 21074, 24915, 28074};

inline bool ft8(uint32_t hz) {
  for (uint32_t f : FT8_KHZ)
    if (hz + 2000 >= f * 1000 && hz <= f * 1000 + 3000) return true;
  return false;
}

inline const char* segmento(uint32_t hz) {
  uint32_t k = hz / 100;
  for (size_t i = 0; i < sizeof(SEGMENTI) / sizeof(SEGMENTI[0]); i++) {
    const Segmento& s = SEGMENTI[i];
    bool ultimo = i + 1 == sizeof(SEGMENTI) / sizeof(SEGMENTI[0]) || SEGMENTI[i + 1].da_khz10 != s.a_khz10;
    if (k >= s.da_khz10 && (k < s.a_khz10 || (ultimo && k == s.a_khz10))) {
      if (!ft8(hz)) return s.nome;
      static char testo[48];
      snprintf(testo, sizeof(testo), "%s · FT8", s.nome);
      return testo;
    }
  }
  return "";
}

}  // namespace bandplan
