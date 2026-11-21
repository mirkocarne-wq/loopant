// WiFi di casa (facoltativo): ora esatta via NTP e indici solari via HTTP.
// Con il WiFi collegato il canale ESP-NOW diventa quello del router: va impostato uguale sull'antenna.
#pragma once
#include <stddef.h>

namespace rete {

void inizia();   // avvia il task di rete (core 0)
void riavvia();  // riconnessione con le credenziali appena salvate
bool collegata();
void stato(char* out, size_t n);  // testo per Impostazioni
void aggiornaIndiciOra();         // richiede subito un nuovo download

}  // namespace rete
