// Band plan IARU Regione 1 semplificato per 40 e 20 m (da verificare sul band plan ufficiale).
#pragma once
#include <stdint.h>

namespace bandplan {

inline const char* segmento(uint32_t hz) {
  // Band plan IARU Regione 1 semplificato: da verificare
  uint32_t k = hz / 1000;
  bool ft8 = (hz >= 7072000 && hz <= 7076000) || (hz >= 14072000 && hz <= 14076000);
  if (k >= 7000 && k < 7040) return "CW";
  if (k >= 7040 && k < 7050) return "Digitale a banda stretta";
  if (k >= 7050 && k <= 7200) return ft8 ? "Tutti i modi · FT8" : "Tutti i modi";
  if (k >= 14000 && k < 14070) return "CW";
  if (k >= 14070 && k < 14099) return ft8 ? "Digitale a banda stretta · FT8" : "Digitale a banda stretta";
  if (k >= 14099 && k < 14101) return "Radiofari";
  if (k >= 14101 && k <= 14350) return "Tutti i modi";
  return "";
}

}  // namespace bandplan
