#include "indici.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARDUINO
#include <Arduino.h>
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define BLOCCA() portENTER_CRITICAL(&mux)
#define SBLOCCA() portEXIT_CRITICAL(&mux)
#else
#define BLOCCA()
#define SBLOCCA()
#endif

namespace indici {
namespace {

Dati attuali = {};

// Valore intero del tag <nome>...</nome>
bool intero(const char* xml, const char* tag, int& out) {
  char apri[24];
  snprintf(apri, sizeof(apri), "<%s>", tag);
  const char* p = strstr(xml, apri);
  if (!p) return false;
  p += strlen(apri);
  while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
  if (*p < '0' || *p > '9') return false;
  out = atoi(p);
  return true;
}

Condizione condizione(const char* testo) {
  if (!strncmp(testo, "Good", 4)) return Condizione::BUONA;
  if (!strncmp(testo, "Fair", 4)) return Condizione::DISCRETA;
  if (!strncmp(testo, "Poor", 4)) return Condizione::SCARSA;
  return Condizione::SCONOSCIUTA;
}

// <band name="80m-40m" time="day">Good</band>
Condizione banda(const char* xml, const char* nome, const char* quando) {
  char cerca[64];
  snprintf(cerca, sizeof(cerca), "name=\"%s\" time=\"%s\">", nome, quando);
  const char* p = strstr(xml, cerca);
  return p ? condizione(p + strlen(cerca)) : Condizione::SCONOSCIUTA;
}

}  // namespace

bool analizza(const char* xml, Dati& out) {
  memset(&out, 0, sizeof(out));
  bool ok = intero(xml, "solarflux", out.sfi);
  ok &= intero(xml, "sunspots", out.ssn);
  ok &= intero(xml, "aindex", out.a);
  ok &= intero(xml, "kindex", out.k);
  for (int i = 0; i < FASCE; i++) {
    out.giorno[i] = banda(xml, FASCIA_FEED[i], "day");
    out.notte[i] = banda(xml, FASCIA_FEED[i], "night");
  }
  out.validi = ok;
  return ok;
}

void imposta(const Dati& d) {
  BLOCCA();
  attuali = d;
  attuali.errore[0] = 0;
  SBLOCCA();
}

void errore(const char* testo) {
  BLOCCA();
  strncpy(attuali.errore, testo, sizeof(attuali.errore) - 1);
  attuali.errore[sizeof(attuali.errore) - 1] = 0;
  SBLOCCA();
}

Dati dati() {
  BLOCCA();
  Dati d = attuali;
  SBLOCCA();
  return d;
}

const char* nome(Condizione c) {
  switch (c) {
    case Condizione::BUONA: return "Buona";
    case Condizione::DISCRETA: return "Discreta";
    case Condizione::SCARSA: return "Scarsa";
    default: return "—";
  }
}

}  // namespace indici
