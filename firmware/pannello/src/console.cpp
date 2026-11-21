#include "console.h"

#include <Arduino.h>
#include <collegamento.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "impostazioni.h"
#include "link_antenna.h"
#include "tema.h"
#include "ui/ui.h"

namespace console {
namespace {

uint32_t leggiFrequenza(const char* s) {
  double v = atof(s);
  if (v <= 0) return 0;
  return v > 100000 ? (uint32_t)v : (uint32_t)(v * 1000.0 + 0.5);  // kHz se < 100000
}

void copiaMaiuscolo(char* dst, size_t n, const char* src) {
  size_t i = 0;
  for (; src[i] && i < n - 1; i++) dst[i] = (char)toupper((unsigned char)src[i]);
  dst[i] = 0;
}

void aiuto() {
  Serial.println(
      "Comandi: goto <kHz> | stop | zero | jog <passi> | cal <kHz> | calcanc <kHz> | stato |\n"
      "         mac <AA:BB:CC:DD:EE:FF> | call <nominativo> | loc <locator> | tema <0-3> |\n"
      "         ora <AAAA-MM-GG HH:MM> (UTC) | aiuto");
}

void stato() {
  link_antenna::Stato s = link_antenna::stato();
  Serial.printf("Collegamento: %s, ultima risposta %lu ms fa, comandi ripetuti %lu/%lu\n",
                s.collegato ? "attivo" : "assente", (unsigned long)s.etaMs, (unsigned long)s.ripetuti,
                (unsigned long)s.inviati);
  if (s.ricevuta)
    Serial.printf("Antenna: %s, errore: %s, posizione %ld, freq %.1f kHz\n", proto::nomeStato(s.r.stato),
                  proto::nomeErrore(s.r.errore), (long)s.r.posizione, s.r.freq_hz / 1000.0);
}

void impostaOra(const char* data, const char* ora) {
  struct tm tm = {};
  if (!data || !ora || sscanf(data, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3 ||
      sscanf(ora, "%d:%d", &tm.tm_hour, &tm.tm_min) != 2) {
    Serial.println("Formato: ora AAAA-MM-GG HH:MM (UTC)");
    return;
  }
  tm.tm_year -= 1900;
  tm.tm_mon -= 1;
  setenv("TZ", "UTC0", 1);
  tzset();
  struct timeval tv = {mktime(&tm), 0};
  settimeofday(&tv, nullptr);
  Serial.println("Ora impostata (UTC)");
}

void esegui(char* riga) {
  char* cmd = strtok(riga, " \t");
  char* a1 = strtok(nullptr, " \t");
  char* a2 = strtok(nullptr, " \t");
  if (!cmd) return;
  Impostazioni& imp = impostazioni::get();

  if (!strcmp(cmd, "goto") && a1) link_antenna::invia(proto::Tipo::GOTO, leggiFrequenza(a1));
  else if (!strcmp(cmd, "stop")) link_antenna::invia(proto::Tipo::STOP);
  else if (!strcmp(cmd, "zero")) link_antenna::invia(proto::Tipo::ZERO);
  else if (!strcmp(cmd, "jog") && a1) link_antenna::invia(proto::Tipo::JOG, 0, atol(a1));
  else if (!strcmp(cmd, "cal") && a1) link_antenna::invia(proto::Tipo::CAL_SALVA, leggiFrequenza(a1));
  else if (!strcmp(cmd, "calcanc") && a1) link_antenna::invia(proto::Tipo::CAL_CANCELLA, leggiFrequenza(a1));
  else if (!strcmp(cmd, "stato")) stato();
  else if (!strcmp(cmd, "mac") && a1) {
    if (!Collegamento::leggiMac(a1, imp.peer)) {
      Serial.println("MAC non valido");
      return;
    }
    impostazioni::salva();
    Serial.println("MAC dell'antenna salvato: riavviare");
  } else if (!strcmp(cmd, "call") && a1) {
    copiaMaiuscolo(imp.nominativo, sizeof(imp.nominativo), a1);
    impostazioni::salva();
    ui::richiediRicostruzione();
  } else if (!strcmp(cmd, "loc") && a1) {
    copiaMaiuscolo(imp.locator, sizeof(imp.locator), a1);
    impostazioni::salva();
    ui::richiediRicostruzione();
  } else if (!strcmp(cmd, "tema") && a1) {
    imp.tema = (uint8_t)atoi(a1) % tema::NUMERO;
    impostazioni::salva();
    ui::richiediRicostruzione();
    Serial.printf("Tema: %s\n", tema::di(imp.tema).nome);
  } else if (!strcmp(cmd, "ora")) impostaOra(a1, a2);
  else aiuto();
}

}  // namespace

void inizia() {
  uint8_t mio[6];
  char testo[18];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, testo);
  Serial.printf("\nloopant - pannello in casa\nMAC di questo pannello: %s\n", testo);
  if (!link_antenna::stato().peerImpostato)
    Serial.println("MAC dell'antenna non impostato: usare 'mac AA:BB:CC:DD:EE:FF'");
  aiuto();
}

void aggiorna() {
  static char riga[64];
  static size_t n = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      riga[n] = 0;
      n = 0;
      esegui(riga);
    } else if (n < sizeof(riga) - 1) {
      riga[n++] = c;
    }
  }
}

}  // namespace console
