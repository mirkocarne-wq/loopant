// Indici solari e condizioni delle bande, scaricati via WiFi (docs/software/09-propagazione.md).
#pragma once
#include <stdint.h>
#include <time.h>

namespace indici {

enum class Condizione : uint8_t { SCONOSCIUTA, BUONA, DISCRETA, SCARSA };

// Fasce del feed (hamqsl.com dà le condizioni a coppie di bande)
constexpr int FASCE = 4;
constexpr const char* FASCIA_FEED[FASCE] = {"80m-40m", "30m-20m", "17m-15m", "12m-10m"};
constexpr const char* FASCIA_NOME[FASCE] = {"80–40 m", "30–20 m", "17–15 m", "12–10 m"};

struct Dati {
  bool validi;
  time_t aggiornato;  // istante dell'ultimo aggiornamento riuscito
  int sfi, ssn, a, k;
  Condizione giorno[FASCE], notte[FASCE];  // per fascia, nell'ordine di FASCIA_FEED
  char errore[48];                 // ultimo errore (vuoto se nessuno)
};

Dati dati();
const char* nome(Condizione c);

// Usate dal modulo rete (WiFi): analisi del feed XML e salvataggio dei dati.
bool analizza(const char* xml, Dati& out);
void imposta(const Dati& d);
void errore(const char* testo);

}  // namespace indici
