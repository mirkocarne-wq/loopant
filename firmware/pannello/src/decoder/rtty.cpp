#include "rtty.h"

#include <math.h>
#include <string.h>

namespace decoder {
namespace {

// ITA2: 0x1B = FIGS, 0x1F = LTRS, 0x08 = CR, 0x02 = LF, 0x04 = spazio
const char LETTERE[32] = {0,   'E', '\n', 'A', ' ', 'S', 'I', 'U', '\r', 'D', 'R', 'J', 'N', 'F', 'C', 'K',
                          'T', 'Z', 'L',  'W', 'H', 'Y', 'P', 'Q', 'O',  'B', 'G', 0,   'M', 'X', 'V', 0};
const char CIFRE[32] = {0,   '3', '\n', '-', ' ', '\'', '8', '7', '\r', '$', '4', '\'', ',', '!', ':', '(',
                        '5', '"', ')',  '2', '#', '6',  '0', '1', '9',  '?', '&', 0,    '.', '/', ';', 0};
constexpr uint8_t FIGS = 0x1B, LTRS = 0x1F, SPAZIO = 0x04;

constexpr float DUE_PI = 6.28318530718f;

}  // namespace

void DecoderRtty::configura(float fs, const Parametri& p, Uscita uscita, void* ctx) {
  fs_ = fs;
  p_ = p;
  uscita_ = uscita;
  ctx_ = ctx;
  bitT_ = fs / p.baud;
  lunghezza_ = (int)lroundf(bitT_ * 0.8f);  // filtro poco più corto del bit
  if (lunghezza_ > 400) lunghezza_ = 400;
  if (lunghezza_ < 8) lunghezza_ = 8;
  memset(buf_, 0, sizeof(buf_));
  indice_ = 0;
  m_ = Canale();
  s_ = Canale();
  m_.bufI = buf_[0];
  m_.bufQ = buf_[1];
  s_.bufI = buf_[2];
  s_.bufQ = buf_[3];
  offset_ = 0;
  contaAfc_ = 0;
  piccoE_ = 1e-9f;
  fase_ = ATTESA;
  ultimoMark_ = true;
  cifre_ = false;
  buoni_ = 0;
  impostaFrequenze();
}

void DecoderRtty::impostaFrequenze() {
  m_.passo = DUE_PI * (p_.mark + offset_) / fs_;
  s_.passo = DUE_PI * (p_.mark + p_.shift + offset_) / fs_;
}

void DecoderRtty::campioni(const int16_t* x, size_t n) {
  for (size_t i = 0; i < n; i++) campione(x[i] / 32768.0f);
}

void DecoderRtty::campione(float x) {
  // miscelazione e media mobile su mark e space
  Canale* canali[2] = {&m_, &s_};
  float energia[2];
  for (int k = 0; k < 2; k++) {
    Canale& c = *canali[k];
    float i = x * cosf(c.fase), q = -x * sinf(c.fase);
    c.fase += c.passo;
    if (c.fase > DUE_PI) c.fase -= DUE_PI;
    c.sommaI += i - c.bufI[indice_];
    c.sommaQ += q - c.bufQ[indice_];
    c.bufI[indice_] = i;
    c.bufQ[indice_] = q;
    energia[k] = c.sommaI * c.sommaI + c.sommaQ * c.sommaQ;
  }
  if (++indice_ >= lunghezza_) indice_ = 0;

  float em = energia[0], es = energia[1];
  float massimo = em > es ? em : es;
  if (massimo > piccoE_) piccoE_ = massimo;
  else piccoE_ *= 0.99995f;
  livMark_ = sqrtf(em / piccoE_);
  livSpace_ = sqrtf(es / piccoE_);

  bool mark = em >= es;
  if (p_.reverse) mark = !mark;

  // AFC: la rotazione di fase del tono attivo misura l'errore di frequenza
  if (p_.afc && massimo > 0.05f * piccoE_) {
    Canale& c = (em >= es) ? m_ : s_;
    float re = c.sommaI * c.precI + c.sommaQ * c.precQ;
    float im = c.sommaQ * c.precI - c.sommaI * c.precQ;
    if (re > 0) {
      c.rotazione += atan2f(im, re);
      c.campioniRot++;
    }
    if (++contaAfc_ >= (int)(fs_ * 0.05f)) {  // ogni 50 ms
      int n = m_.campioniRot + s_.campioniRot;
      if (n > 100) {
        float errore = (m_.rotazione + s_.rotazione) / n * fs_ / DUE_PI;
        offset_ += 0.3f * errore;
        if (offset_ > 100) offset_ = 100;
        if (offset_ < -100) offset_ = -100;
        impostaFrequenze();
      }
      m_.rotazione = s_.rotazione = 0;
      m_.campioniRot = s_.campioniRot = 0;
      contaAfc_ = 0;
    }
  }
  m_.precI = m_.sommaI;
  m_.precQ = m_.sommaQ;
  s_.precI = s_.sommaI;
  s_.precQ = s_.sommaQ;

  // il filtro ritarda tutte le transizioni allo stesso modo: si lavora sul segnale filtrato
  bool segnale = massimo > 0.02f * piccoE_;
  switch (fase_) {
    case ATTESA:
      if (segnale && ultimoMark_ && !mark) {  // fronte mark -> space: bit di start
        fase_ = START;
        t_ = 0;
      }
      break;
    case START:
      t_ += 1;
      if (t_ >= 0.5f * bitT_) {
        if (mark) fase_ = ATTESA;  // disturbo
        else {
          fase_ = DATI;
          nBit_ = 0;
          codice_ = 0;
        }
      }
      break;
    case DATI:
      t_ += 1;
      if (t_ >= (1.5f + nBit_) * bitT_) {
        if (mark) codice_ |= (uint8_t)(1 << nBit_);  // LSB per primo
        if (++nBit_ == 5) fase_ = STOP;
      }
      break;
    case STOP:
      t_ += 1;
      if (t_ >= 6.5f * bitT_) {
        if (mark) {
          carattere(codice_);
          if (buoni_ < 10) buoni_++;
        } else if (buoni_ > 0) {
          buoni_--;  // errore di framing
        }
        fase_ = ATTESA;
      }
      break;
  }
  ultimoMark_ = mark;
}

void DecoderRtty::carattere(uint8_t codice) {
  if (codice == FIGS) {
    cifre_ = true;
    return;
  }
  if (codice == LTRS) {
    cifre_ = false;
    return;
  }
  if (codice == SPAZIO && p_.uos) cifre_ = false;
  char c = cifre_ ? CIFRE[codice & 0x1F] : LETTERE[codice & 0x1F];
  if (c == '\r' || !c) return;  // il ritorno carrello si ignora, LF va a capo
  if (uscita_) uscita_(c, ctx_);
}

}  // namespace decoder
