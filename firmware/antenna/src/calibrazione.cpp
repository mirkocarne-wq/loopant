#include "calibrazione.h"

#include <Preferences.h>

namespace calibrazione {
namespace {

struct Punto {
  uint32_t freq_hz;
  int32_t posizione;
};

struct Tabella {
  uint8_t n;
  Punto p[MAX_PUNTI];  // ordinati per frequenza crescente
};

Tabella tabelle[bande::NUMERO];
Preferences nvs;

int indice(bande::Banda b) {
  for (size_t i = 0; i < bande::NUMERO; i++)
    if (bande::ELENCO[i].banda == b) return (int)i;
  return -1;
}

void chiave(int i, char out[8]) { snprintf(out, 8, "cal%u", (unsigned)bande::ELENCO[i].banda); }

void scrivi(int i) {
  char k[8];
  chiave(i, k);
  nvs.putBytes(k, &tabelle[i], sizeof(Tabella));
}

double x(uint32_t f) {  // 1/f² con f in MHz
  double mhz = f / 1e6;
  return 1.0 / (mhz * mhz);
}

}  // namespace

void inizia() {
  nvs.begin("calibra", false);
  for (size_t i = 0; i < bande::NUMERO; i++) {
    char k[8];
    chiave(i, k);
    if (nvs.getBytes(k, &tabelle[i], sizeof(Tabella)) != sizeof(Tabella) ||
        tabelle[i].n > MAX_PUNTI)
      tabelle[i].n = 0;
  }
}

bool salvaPunto(uint32_t f, int32_t pos) {
  int i = indice(bande::di(f));
  if (i < 0) return false;
  Tabella& t = tabelle[i];

  // sostituisce un punto vicino, altrimenti inserisce in ordine
  for (uint8_t j = 0; j < t.n; j++) {
    if ((uint32_t)abs((int32_t)(t.p[j].freq_hz - f)) <= TOLLERANZA_HZ) {
      t.p[j] = {f, pos};
      scrivi(i);
      return true;
    }
  }
  if (t.n >= MAX_PUNTI) return false;
  uint8_t j = t.n;
  while (j > 0 && t.p[j - 1].freq_hz > f) {
    t.p[j] = t.p[j - 1];
    j--;
  }
  t.p[j] = {f, pos};
  t.n++;
  scrivi(i);
  return true;
}

void cancella(bande::Banda b) {
  int i = indice(b);
  if (i < 0) return;
  tabelle[i].n = 0;
  scrivi(i);
}

bool calibrata(bande::Banda b) {
  int i = indice(b);
  return i >= 0 && tabelle[i].n >= 2;
}

bool posizionePer(uint32_t f, int32_t& pos) {
  int i = indice(bande::di(f));
  if (i < 0 || tabelle[i].n < 2) return false;
  const Tabella& t = tabelle[i];

  // segmento che contiene f (o il primo/ultimo per estrapolare)
  uint8_t j = 1;
  while (j < t.n - 1 && t.p[j].freq_hz < f) j++;
  const Punto& a = t.p[j - 1];
  const Punto& b = t.p[j];
  double xa = x(a.freq_hz), xb = x(b.freq_hz);
  if (xa == xb) return false;
  double k = (x(f) - xa) / (xb - xa);
  pos = (int32_t)lround(a.posizione + k * (b.posizione - a.posizione));
  return true;
}

void stampa(Print& out) {
  for (size_t i = 0; i < bande::NUMERO; i++) {
    const Tabella& t = tabelle[i];
    out.printf("Banda %s: %u punti%s\n", bande::ELENCO[i].nome, t.n,
               t.n >= 2 ? "" : " (non calibrata)");
    for (uint8_t j = 0; j < t.n; j++)
      out.printf("  %10.1f kHz -> %ld\n", t.p[j].freq_hz / 1000.0, (long)t.p[j].posizione);
  }
}

}  // namespace calibrazione
