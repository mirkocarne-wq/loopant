#include "ora.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "impostazioni.h"

namespace ora {
namespace {

const char* FUSI[] = {
    "UTC0",                          // 0: UTC
    "CET-1CEST,M3.5.0,M10.5.0/3",    // 1: Europa centrale (Italia)
};

const char* GIORNI[] = {"domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"};
const char* MESI[] = {"gennaio", "febbraio", "marzo",     "aprile",  "maggio",   "giugno",
                      "luglio",  "agosto",   "settembre", "ottobre", "novembre", "dicembre"};

}  // namespace

void applicaFuso() {
  uint8_t f = impostazioni::get().fuso;
  setenv("TZ", FUSI[f < 2 ? f : 1], 1);
  tzset();
}

bool valida() { return time(nullptr) > 1700000000; }

void hhmmUtc(char* out, size_t n) {
  time_t t = time(nullptr);
  struct tm u;
  gmtime_r(&t, &u);
  snprintf(out, n, "%02d:%02d", u.tm_hour, u.tm_min);
}

void hhmmLocale(char* out, size_t n) {
  time_t t = time(nullptr);
  struct tm l;
  localtime_r(&t, &l);
  snprintf(out, n, "%02d:%02d", l.tm_hour, l.tm_min);
}

void dataLocale(char* out, size_t n) {
  time_t t = time(nullptr);
  struct tm l;
  localtime_r(&t, &l);
  snprintf(out, n, "%s %d %s", GIORNI[l.tm_wday % 7], l.tm_mday, MESI[l.tm_mon % 12]);
}

bool impostaUtc(int anno, int mese, int giorno, int ore, int minuti) {
  if (anno < 2024 || mese < 1 || mese > 12 || giorno < 1 || giorno > 31 || ore < 0 || ore > 23 || minuti < 0 ||
      minuti > 59)
    return false;
  struct tm tm = {};
  tm.tm_year = anno - 1900;
  tm.tm_mon = mese - 1;
  tm.tm_mday = giorno;
  tm.tm_hour = ore;
  tm.tm_min = minuti;
  setenv("TZ", "UTC0", 1);  // mktime lavora nel fuso corrente
  tzset();
  struct timeval tv = {mktime(&tm), 0};
  settimeofday(&tv, nullptr);
  applicaFuso();
  return true;
}

}  // namespace ora
