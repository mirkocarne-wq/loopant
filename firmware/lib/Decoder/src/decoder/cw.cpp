#include "cw.h"

#include <math.h>
#include <string.h>

namespace decoder {
namespace {

struct Simbolo {
  const char* codice;
  char c;
};

const Simbolo MORSE[] = {
    {".-", 'A'},     {"-...", 'B'},   {"-.-.", 'C'},   {"-..", 'D'},    {".", 'E'},      {"..-.", 'F'},
    {"--.", 'G'},    {"....", 'H'},   {"..", 'I'},     {".---", 'J'},   {"-.-", 'K'},    {".-..", 'L'},
    {"--", 'M'},     {"-.", 'N'},     {"---", 'O'},    {".--.", 'P'},   {"--.-", 'Q'},   {".-.", 'R'},
    {"...", 'S'},    {"-", 'T'},      {"..-", 'U'},    {"...-", 'V'},   {".--", 'W'},    {"-..-", 'X'},
    {"-.--", 'Y'},   {"--..", 'Z'},   {"-----", '0'},  {".----", '1'},  {"..---", '2'},  {"...--", '3'},
    {"....-", '4'},  {".....", '5'},  {"-....", '6'},  {"--...", '7'},  {"---..", '8'},  {"----.", '9'},
    {".-.-.-", '.'}, {"--..--", ','}, {"..--..", '?'}, {"-..-.", '/'},  {"-...-", '='},  {".-.-.", '+'},
    {"-....-", '-'}, {"---...", ':'}, {".----.", '\''}, {"-.--.", '('}, {"-.--.-", ')'}, {".--.-.", '@'},
};

char decodifica(const char* codice) {
  for (const Simbolo& s : MORSE)
    if (!strcmp(s.codice, codice)) return s.c;
  return '*';  // carattere non riconosciuto
}

}  // namespace

void DecoderCw::configura(float fs, float tono, float filtro, uint8_t wpmFisso, Uscita uscita, void* ctx) {
  fs_ = fs;
  tono_ = tono;
  // blocco con un numero intero di periodi del tono: altrimenti l'ampiezza misurata
  // oscilla con la fase (dispersione della frequenza negativa) e i segni si spezzano
  float periodi = roundf(fs / filtro * tono / fs);
  if (periodi < 1) periodi = 1;
  n_ = (int)lroundf(periodi * fs / tono);
  if (n_ < 8) n_ = 8;
  msBlocco_ = 1000.0f * n_ / fs;
  coeff_ = 2.0f * cosf(2.0f * (float)M_PI * tono / fs);
  conta_ = 0;
  s1_ = s2_ = 0;
  picco_ = fondo_ = 0;
  riscaldamento_ = 0;
  tasto_ = candidato_ = false;
  stabili_ = 0;
  durata_ = 0;
  wpmFisso_ = wpmFisso != 0;
  dit_ = 1200.0f / (wpmFisso ? wpmFisso : 20);
  nElementi_ = 0;
  elementi_[0] = 0;
  spazioEmesso_ = true;
  uscita_ = uscita;
  ctx_ = ctx;
}

void DecoderCw::campioni(const int16_t* x, size_t n) {
  for (size_t i = 0; i < n; i++) {
    float s = x[i] / 32768.0f + coeff_ * s1_ - s2_;
    s2_ = s1_;
    s1_ = s;
    if (++conta_ >= n_) {
      float p = s1_ * s1_ + s2_ * s2_ - coeff_ * s1_ * s2_;
      blocco(sqrtf(p > 0 ? p : 0) / n_);
      conta_ = 0;
      s1_ = s2_ = 0;
    }
  }
}

void DecoderCw::emetti(char c) {
  if (uscita_) uscita_(c, ctx_);
}

void DecoderCw::chiudiCarattere() {
  if (!nElementi_) return;
  emetti(decodifica(elementi_));
  nElementi_ = 0;
  elementi_[0] = 0;
  spazioEmesso_ = false;
}

void DecoderCw::fineSegno(float ms) {
  if (nElementi_ < sizeof(elementi_) - 1) {
    bool punto = ms < 2.0f * dit_;
    elementi_[nElementi_++] = punto ? '.' : '-';
    elementi_[nElementi_] = 0;
    if (!wpmFisso_) {
      float stima = punto ? ms : ms / 3.0f;
      dit_ = 0.8f * dit_ + 0.2f * stima;
      if (dit_ < 24) dit_ = 24;    // 50 WPM
      if (dit_ > 240) dit_ = 240;  // 5 WPM
    }
  }
}

void DecoderCw::blocco(float mag) {
  // riscaldamento: i primi blocchi servono solo a stimare il rumore
  if (riscaldamento_ < 32) {
    riscaldamento_++;
    fondo_ += (mag - fondo_) / riscaldamento_;
    return;
  }
  // inviluppo: picco del tono con discesa lenta; rumore = media dell'ampiezza a tasto alzato
  const float discesa = expf(-msBlocco_ / 3000.0f);
  if (mag > picco_) picco_ = mag;
  else picco_ *= discesa;
  if (fondo_ == 0) fondo_ = mag;
  else if (!tasto_) {
    // i blocchi molto sopra il rumore sono tono (es. i blocchi di conferma all'inizio di un segno):
    // contano pochissimo, così la stima del rumore non sale con il segnale
    float alfa = msBlocco_ / 400.0f;
    if (mag > 3.0f * fondo_) alfa *= 0.02f;
    fondo_ += (mag - fondo_) * alfa;
  }

  float escursione = picco_ - fondo_;
  // squelch: il picco deve stare ben sopra il rumore (il massimo del solo rumore è circa 3-4 volte la media)
  bool segnale = picco_ > 6.0f * fondo_ && picco_ > 1e-4f;
  livello_ = picco_ > 0 ? mag / picco_ : 0;
  soglia_ = picco_ > 0 ? (fondo_ + 0.45f * escursione) / picco_ : 0.5f;

  // isteresi: si accende sopra il 55 % dell'escursione, si spegne sotto il 35 %
  bool acceso = tasto_ ? mag > fondo_ + 0.35f * escursione : mag > fondo_ + 0.55f * escursione;
  if (!segnale) acceso = false;

  // un cambio di stato vale solo se dura almeno 2 blocchi
  if (acceso != tasto_) {
    if (acceso == candidato_) stabili_++;
    else {
      candidato_ = acceso;
      stabili_ = 1;
    }
    if (stabili_ >= 2) {
      float ms = durata_;  // durata dello stato che finisce
      tasto_ = acceso;
      durata_ = msBlocco_ * stabili_;
      stabili_ = 0;
      if (!tasto_) {
        fineSegno(ms);
      } else {
        if (ms >= 5.0f * dit_) {
          chiudiCarattere();
          if (!spazioEmesso_) {
            emetti(' ');
            spazioEmesso_ = true;
          }
        } else if (ms >= 2.0f * dit_) {
          chiudiCarattere();
        }
      }
      return;
    }
  } else {
    stabili_ = 0;
    candidato_ = tasto_;
  }
  durata_ += msBlocco_;

  // silenzio lungo: chiude carattere e parola senza aspettare il segno successivo
  if (!tasto_) {
    if (durata_ >= 2.5f * dit_) chiudiCarattere();
    if (durata_ >= 7.0f * dit_ && !spazioEmesso_) {
      emetti(' ');
      spazioEmesso_ = true;
    }
  }
}

}  // namespace decoder
