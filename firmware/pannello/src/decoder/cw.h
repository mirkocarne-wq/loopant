// Decoder CW: Goertzel sul tono, soglia adattiva, stima automatica della velocità.
// C++ puro (nessuna dipendenza da Arduino): si prova sul PC con test/test_decoder.cpp.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace decoder {

class DecoderCw {
 public:
  typedef void (*Uscita)(char c, void* ctx);

  void configura(float fs, float tono, float larghezzaFiltro, uint8_t wpmFisso, Uscita uscita, void* ctx);
  void campioni(const int16_t* x, size_t n);

  // Stato per l'interfaccia
  float livello() const { return livello_; }  // 0-1 rispetto al picco recente
  float soglia() const { return soglia_; }    // 0-1
  float wpm() const { return 1200.0f / dit_; }
  bool tasto() const { return tasto_; }
  const char* elementi() const { return elementi_; }  // ultimi punti/linee del carattere in corso

 private:
  void blocco(float mag);
  void fineSegno(float ms);
  void emetti(char c);
  void chiudiCarattere();

  float fs_ = 8000, tono_ = 700, msBlocco_ = 4;
  int n_ = 32;
  float coeff_ = 0;
  int conta_ = 0;
  float s1_ = 0, s2_ = 0;

  float picco_ = 0, fondo_ = 0;
  int riscaldamento_ = 0;
  float livello_ = 0, soglia_ = 0.5f;
  bool tasto_ = false, candidato_ = false;
  int stabili_ = 0;
  float durata_ = 0;  // ms nello stato attuale

  float dit_ = 60;  // ms
  bool wpmFisso_ = false;
  char elementi_[8] = "";
  uint8_t nElementi_ = 0;
  bool spazioEmesso_ = true;

  Uscita uscita_ = nullptr;
  void* ctx_ = nullptr;
};

}  // namespace decoder
