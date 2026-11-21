// Segnale di prova per la manopola (sorgente "prova"): CW o RTTY sintetici a 8 kHz con un po' di rumore,
// generati in tempo reale. Serve a provare decoder e collegamento senza la radio.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace generatore {

struct Parametri {
  bool rtty = false;
  float fs = 8000;
  // CW
  float tono = 700, wpm = 18;
  // RTTY
  float baud = 45.45f, mark = 2125, shift = 170, bitStop = 1.5f;
};

void configura(const Parametri& p);
void campioni(int16_t* x, size_t n);  // riempie x con i prossimi n campioni

}  // namespace generatore
