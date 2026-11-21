// Prova del decimatore 32 kHz -> 8 kHz: toni nella banda (CW, RTTY) e fuori banda (che si ripiegherebbero).
#include <math.h>
#include <stdio.h>

#include "decoder/decimatore.h"

static int errori = 0;

static void verifica(bool ok, const char* testo) {
  printf("%s %s\n", ok ? "OK  " : "ERR ", testo);
  if (!ok) errori++;
}

// Ampiezza di picco in uscita per un tono di ampiezza 1 a 'f' Hz (dopo il transitorio)
static double ampiezza(double f) {
  decoder::Decimatore d;
  d.configura(32000, 3600);
  double picco = 0;
  for (int n = 0; n < 32000; n++) {
    float y;
    if (d.campione((float)sin(2 * M_PI * f * n / 32000.0), y) && n > 4000 && fabs(y) > picco) picco = fabs(y);
  }
  return picco;
}

int main() {
  printf("== Decimatore ==\n");
  struct { double f; double minDb, maxDb; const char* nome; } casi[] = {
      {700, -0.3, 0.3, "CW 700 Hz in banda"},
      {2125, -0.5, 0.5, "RTTY mark 2125 Hz in banda"},
      {2295, -0.5, 0.5, "RTTY space 2295 Hz in banda"},
      {6000, -200, -40, "6 kHz (si ripiegherebbe a 2 kHz) attenuato oltre 40 dB"},
      {7300, -200, -40, "7,3 kHz (si ripiegherebbe a 700 Hz) attenuato oltre 40 dB"},
      {12000, -200, -40, "12 kHz attenuato oltre 40 dB"},
  };
  for (auto& c : casi) {
    double db = 20 * log10(ampiezza(c.f) + 1e-12);
    char t[120];
    snprintf(t, sizeof(t), "%s (%.1f dB)", c.nome, db);
    verifica(db >= c.minDb && db <= c.maxDb, t);
  }
  printf(errori ? "\n%d test falliti\n" : "\nTutti i test superati\n", errori);
  return errori ? 1 : 0;
}
