// Test sul PC dell'analisi del feed degli indici solari (formato hamqsl.com, da confermare sul feed reale).
// g++ -std=c++17 -I../src test_indici.cpp ../src/indici.cpp -o test_indici && ./test_indici
#include <stdio.h>

#include "indici.h"

static const char* ESEMPIO =
    "<solar><solardata><source url=\"http://www.hamqsl.com/solar.html\">N0NBH</source>"
    "<updated> 21 Nov 2026 0700 GMT</updated><solarflux>145</solarflux><aindex> 8</aindex>"
    "<kindex> 2</kindex><sunspots>98</sunspots><calculatedconditions>"
    "<band name=\"80m-40m\" time=\"day\">Fair</band><band name=\"30m-20m\" time=\"day\">Good</band>"
    "<band name=\"80m-40m\" time=\"night\">Good</band><band name=\"30m-20m\" time=\"night\">Poor</band>"
    "</calculatedconditions></solardata></solar>";

int main() {
  int errori = 0;
  auto verifica = [&](bool ok, const char* cosa) {
    printf("%s %s\n", ok ? "OK  " : "ERRORE", cosa);
    if (!ok) errori++;
  };
  indici::Dati d;
  verifica(indici::analizza(ESEMPIO, d), "feed di esempio riconosciuto");
  verifica(d.sfi == 145 && d.ssn == 98 && d.a == 8 && d.k == 2, "SFI 145, SSN 98, A 8, K 2");
  verifica(d.b40Giorno == indici::Condizione::DISCRETA && d.b40Notte == indici::Condizione::BUONA, "40 m: discreta di giorno, buona di notte");
  verifica(d.b20Giorno == indici::Condizione::BUONA && d.b20Notte == indici::Condizione::SCARSA, "20 m: buona di giorno, scarsa di notte");
  verifica(!indici::analizza("<html>errore</html>", d), "pagina non valida rifiutata");
  printf(errori ? "\n%d ERRORI\n" : "\nTutti i test superati\n", errori);
  return errori ? 1 : 0;
}
