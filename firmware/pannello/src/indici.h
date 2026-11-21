// Indici solari e condizioni delle bande, scaricati via WiFi (docs/software/09-propagazione.md).
#pragma once
#include <stdint.h>
#include <time.h>

namespace indici {

enum class Condizione : uint8_t { SCONOSCIUTA, BUONA, DISCRETA, SCARSA };

struct Dati {
  bool validi;
  time_t aggiornato;  // istante dell'ultimo aggiornamento riuscito
  int sfi, ssn, a, k;
  Condizione b40Giorno, b40Notte;  // fascia "80m-40m" del feed
  Condizione b20Giorno, b20Notte;  // fascia "30m-20m" del feed
  char errore[48];                 // ultimo errore (vuoto se nessuno)
};

Dati dati();
const char* nome(Condizione c);

// Usate dal modulo rete (WiFi): analisi del feed XML e salvataggio dei dati.
bool analizza(const char* xml, Dati& out);
void imposta(const Dati& d);
void errore(const char* testo);

}  // namespace indici
