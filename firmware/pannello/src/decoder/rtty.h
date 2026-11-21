// Decoder RTTY (Baudot ITA2): due filtri su mark e space, campionamento a metà bit, AFC.
// C++ puro (nessuna dipendenza da Arduino): si prova sul PC con test/test_decoder.cpp.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace decoder {

class DecoderRtty {
 public:
  typedef void (*Uscita)(char c, void* ctx);

  struct Parametri {
    float baud = 45.45f;
    float shift = 170;
    float mark = 2125;   // space = mark + shift
    float bitStop = 1.5f;
    bool reverse = false;
    bool uos = true;     // unshift on space
    bool afc = true;
  };

  void configura(float fs, const Parametri& p, Uscita uscita, void* ctx);
  void campioni(const int16_t* x, size_t n);

  // Stato per l'interfaccia
  float livelloMark() const { return livMark_; }    // 0-1
  float livelloSpace() const { return livSpace_; }  // 0-1
  float offsetAfc() const { return offset_; }       // Hz
  bool agganciato() const { return buoni_ >= 3; }

 private:
  struct Canale {
    float fase = 0, passo = 0;
    float* bufI = nullptr;
    float* bufQ = nullptr;
    float sommaI = 0, sommaQ = 0;
    float precI = 0, precQ = 0;
    float rotazione = 0;  // somma delle rotazioni di fase (AFC)
    int campioniRot = 0;
  };

  void impostaFrequenze();
  void campione(float x);
  void carattere(uint8_t codice);

  float fs_ = 8000;
  Parametri p_;
  int lunghezza_ = 176;  // campioni per bit (filtro a media mobile)
  float buf_[4][400];
  int indice_ = 0;
  Canale m_, s_;

  float offset_ = 0;
  int contaAfc_ = 0;
  float piccoE_ = 1e-9f;
  float livMark_ = 0, livSpace_ = 0;

  // macchina a stati del carattere
  enum Fase : uint8_t { ATTESA, START, DATI, STOP };
  Fase fase_ = ATTESA;
  bool ultimoMark_ = true;
  float t_ = 0;          // campioni dall'inizio del bit di start
  float bitT_ = 176;     // campioni per bit
  uint8_t nBit_ = 0;
  uint8_t codice_ = 0;
  bool cifre_ = false;   // FIGS
  int buoni_ = 0;

  Uscita uscita_ = nullptr;
  void* ctx_ = nullptr;
};

}  // namespace decoder
