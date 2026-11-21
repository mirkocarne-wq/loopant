#include "decimatore.h"

#include <math.h>

namespace decoder {

void Decimatore::configura(float fsIngresso, float taglio) {
  const float pi = 3.14159265358979f;
  const float fc = taglio / fsIngresso;  // frequenza normalizzata
  const int m = PRESE - 1;
  float somma = 0;
  for (int i = 0; i < PRESE; i++) {
    float k = (float)i - m / 2.0f;
    float sinc = k == 0 ? 2 * fc : sinf(2 * pi * fc * k) / (pi * k);
    float finestra = 0.54f - 0.46f * cosf(2 * pi * i / m);  // Hamming
    h_[i] = sinc * finestra;
    somma += h_[i];
  }
  for (int i = 0; i < PRESE; i++) h_[i] /= somma;  // guadagno 1 in continua
  for (int i = 0; i < PRESE; i++) storia_[i] = 0;
  pos_ = 0;
  conta_ = 0;
}

bool Decimatore::campione(float x, float& y) {
  storia_[pos_] = x;
  pos_ = (pos_ + 1) % PRESE;
  if (++conta_ < FATTORE) return false;
  conta_ = 0;
  // il filtro si calcola solo per i campioni che escono
  float s = 0;
  int j = pos_;  // il più vecchio
  for (int i = PRESE - 1; i >= 0; i--) {
    s += h_[i] * storia_[j];
    j = (j + 1) % PRESE;
  }
  y = s;
  return true;
}

}  // namespace decoder
