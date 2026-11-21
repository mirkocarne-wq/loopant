// Test sul PC dei calcoli del sole e del locator.
// g++ -std=c++17 -I../src test_sole.cpp ../src/sole.cpp ../src/locator.cpp -o test_sole && ./test_sole
#include <math.h>
#include <stdio.h>
#include <time.h>

#include "locator.h"
#include "sole.h"

static int errori = 0;
static void verifica(bool ok, const char* cosa) {
  printf("%s %s\n", ok ? "OK  " : "ERRORE", cosa);
  if (!ok) errori++;
}

static time_t utc(int a, int m, int g, int h, int mi) {
  struct tm t = {};
  t.tm_year = a - 1900; t.tm_mon = m - 1; t.tm_mday = g; t.tm_hour = h; t.tm_min = mi;
  return timegm(&t);
}

static void hhmm(double h, char* b) { int m = (int)lround(h * 60); sprintf(b, "%02d:%02d", m / 60 % 24, m % 60); }

int main() {
  double lat, lon;
  verifica(locator::aCoordinate("JN61", lat, lon) && fabs(lat - 41.5) < 1e-9 && fabs(lon - 13.0) < 1e-9, "JN61 -> 41,5 N 13,0 E");
  verifica(locator::aCoordinate("JN61fv", lat, lon) && fabs(lat - 41.896) < 0.001 && fabs(lon - 12.458) < 0.001, "JN61fv -> 41,896 N 12,458 E (Roma)");
  verifica(!locator::aCoordinate("ZZ99", lat, lon), "locator non valido rifiutato");
  verifica(locator::aCoordinate("FN31", lat, lon) && lat > 41 && lat < 42 && lon > -74 && lon < -72, "FN31 -> New York");

  // Roma, 20/11/2026: riferimento indicativo circa 07:05 / 16:44 ora locale = 06:05 / 15:44 UTC (da verificare)
  time_t t = utc(2026, 11, 20, 14, 32);
  sole::AlbaTramonto at = sole::albaTramonto(t, 41.9, 12.5);
  char a[8], b[8];
  hhmm(at.alba, a); hhmm(at.tramonto, b);
  printf("     Roma 20/11/2026: alba %s, tramonto %s UTC\n", a, b);
  verifica(fabs(at.alba - (6 + 5 / 60.0)) < 0.25, "alba entro 15 min dal riferimento");
  verifica(fabs(at.tramonto - (15 + 44 / 60.0)) < 0.25, "tramonto entro 15 min dal riferimento");

  sole::Posizione p = sole::calcola(t);
  printf("     declinazione %.2f, lon subsolare %.2f, EoT %.1f min\n", p.declinazione, p.lonSubsolare, p.eot);
  verifica(p.declinazione < -19 && p.declinazione > -21, "declinazione di novembre circa -20");
  verifica(!sole::notte(p, 0, p.lonSubsolare), "giorno al punto subsolare");
  verifica(sole::notte(p, 0, p.lonSubsolare + 180), "notte agli antipodi");
  verifica(sole::notte(p, 85, p.lonSubsolare), "notte polare a nord in novembre");

  // Solstizio d'estate a Capo Nord: sempre giorno
  verifica(sole::albaTramonto(utc(2026, 6, 21, 12, 0), 71.0, 25.8).sempreGiorno, "sole di mezzanotte a Capo Nord");
  printf(errori ? "\n%d ERRORI\n" : "\nTutti i test superati\n", errori);
  return errori ? 1 : 0;
}
