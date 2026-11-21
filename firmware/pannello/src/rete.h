// WiFi di casa (facoltativo): ora esatta via NTP e indici solari via HTTP.
// Con il WiFi collegato il canale ESP-NOW diventa quello del router: va impostato uguale sull'antenna.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace rete {

void inizia();   // avvia il task di rete (core 0)
void riavvia();  // riconnessione con le credenziali appena salvate
bool collegata();
void stato(char* out, size_t n);  // testo per Impostazioni
void aggiornaIndiciOra();         // richiede subito un nuovo download

// Salva rete e password nelle impostazioni e si collega.
void collega(const char* ssid, const char* password);

// --- Ricerca delle reti ------------------------------------------------------------------------
// Durante la ricerca la radio cambia canale: il collegamento con l'antenna si interrompe per qualche secondo.

struct Rete {
  char ssid[33];
  int8_t rssi;  // dBm
  bool protetta;
  uint8_t canale;
};
constexpr uint8_t MAX_RETI = 16;

void avviaScansione();
// Copia gli ultimi risultati (dal segnale più forte) e restituisce quante reti ci sono.
// inCorso: ricerca non ancora finita; versione: cambia a ogni ricerca completata.
uint8_t reti(Rete* out, uint8_t max, bool& inCorso, uint32_t& versione);

}  // namespace rete
