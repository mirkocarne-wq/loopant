// Display OLED 0,96" dell'unità antenna (SSD1306 128x64, I2C; prime 16 righe gialle, le altre blu).
// Mostra stato, frequenza, posizione e collegamento con il pannello. Si disegna in un task a bassa priorità
// sul core 0, così il loop (che legge i finecorsa) non aspetta mai l'I2C. Facoltativo: senza display non fa nulla.
#pragma once
#include <stdint.h>

#include "protocollo.h"

namespace oled {

struct Dati {
  proto::Stato stato;
  proto::Errore errore;
  uint32_t freqHz;     // 0 = frequenza non nota
  int32_t posizione;   // passi
  bool driver;         // driver del motore abilitato
  bool can;            // motore comandato via CAN (solo RX)
  bool peerValido;     // MAC del pannello impostato
  bool collegato;      // messaggi dal pannello negli ultimi secondi
  int8_t rssi;         // dBm dell'ultimo messaggio; 0 = non disponibile
  uint8_t associa;     // associazione::Fase
  uint8_t canale;      // canale ESP-NOW (in prova durante l'associazione)
  char pannello[18];   // MAC del pannello associato
};

bool inizia(const char* macLocale, uint8_t canale);  // false se il display non risponde
bool presente();
void aggiorna(const Dati& d);        // dal loop; un cambio di stato, errore o frequenza riaccende il display
void sveglia();                      // riaccende il display (per esempio a un comando ricevuto)

}  // namespace oled
