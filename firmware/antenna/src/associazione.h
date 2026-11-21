// Associazione con il pannello tramite il pulsante BOOT (GPIO 0) del DevKit.
// Pressione di 3 s: l'antenna invia richieste in broadcast canale per canale finché un pannello risponde,
// poi resta sul suo canale e aspetta la conferma dell'utente sul pannello. Pressione breve: annulla la ricerca,
// altrimenti riaccende il display. Non premere BOOT durante l'accensione: la scheda entra in programmazione.
#pragma once
#include <stdint.h>

#include "collegamento.h"

namespace associazione {

enum class Fase : uint8_t {
  INATTIVA,
  RICERCA,    // richieste su tutti i canali
  ATTESA,     // il pannello ha risposto: si aspetta il tocco su "Associa"
  RIUSCITA,   // (mostrate per qualche secondo, poi INATTIVA)
  RIFIUTATA,
  FALLITA,    // nessun pannello entro il tempo massimo, o annullata
};

void inizia();
void avvia();  // come la pressione lunga (comando "associa" della console)

// Dal loop. 'consentita': motore fermo (la ricerca cambia canale e interrompe i comandi).
// Restituisce true una volta, quando il pannello ha confermato: in 'mac' e 'canale' il pannello e il suo canale.
bool aggiorna(Collegamento& radio, bool consentita, uint8_t mac[6], uint8_t& canale);

Fase fase();
uint8_t canale();       // canale in prova o trovato
bool pressioneBreve();  // true una volta dopo una pressione breve fuori dalla ricerca

}  // namespace associazione
