// Decimatore per l'ingresso audio: l'ADC campiona a 4 volte la frequenza del decoder (32 kHz) e un FIR passa-basso
// (finestra di Hamming) riduce a 8 kHz. Toglie quello che sopra i 4 kHz si ripiegherebbe sui toni (aliasing) e
// media il rumore dell'ADC (circa un bit in più). C++ puro: si prova sul PC con test/test_decimatore.cpp.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace decoder {

class Decimatore {
 public:
  static constexpr int FATTORE = 4;
  static constexpr int PRESE = 63;  // banda passante piatta fino a circa 2,8 kHz, attenuata da circa 4,4 kHz

  // fsIngresso: frequenza dell'ADC; taglio: frequenza a metà ampiezza (-6 dB) del filtro
  void configura(float fsIngresso, float taglio);
  // Un campione in ingresso; true quando è pronto un campione in uscita (uno ogni FATTORE)
  bool campione(float x, float& y);

 private:
  float h_[PRESE] = {};
  float storia_[PRESE] = {};
  int pos_ = 0;
  int conta_ = 0;
};

}  // namespace decoder
