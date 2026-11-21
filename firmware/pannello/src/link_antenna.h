// Collegamento ESP-NOW con l'unità antenna: comandi, heartbeat, ripetizioni, stato,
// lettura della tabella di calibrazione.
// aggiorna() va chiamata dal loop(); le altre funzioni si possono chiamare anche dal task di LVGL.
#pragma once
#include <bande.h>
#include <protocollo.h>
#include <stdint.h>

namespace link_antenna {

constexpr uint8_t MAX_PUNTI = 16;

struct Stato {
  bool collegato;        // risposte ricevute negli ultimi TIMEOUT_COLLEGAMENTO_MS
  bool ricevuta;         // almeno una risposta dall'accensione
  bool peerImpostato;    // MAC dell'antenna impostato
  bool comandoFallito;   // l'ultimo comando non ha avuto risposta dopo MAX_TENTATIVI
  proto::Risposta r;     // ultima risposta di stato
  uint32_t etaMs;        // tempo dall'ultima risposta
  int8_t rssi;           // dBm dell'ultimo pacchetto (0 = sconosciuto)
  uint32_t ripetuti;     // comandi ripetuti per mancata risposta
  uint32_t inviati;
  uint32_t ricevuti;
};

struct Tabella {
  bande::Banda banda;
  bool completa;        // tutti i punti letti
  uint8_t totale;
  uint32_t freq_hz[MAX_PUNTI];
  int32_t posizione[MAX_PUNTI];
  uint32_t versione;    // cambia a ogni aggiornamento
};

// Associazione: un'antenna con BOOT premuto chiede di associarsi e aspetta la conferma dell'utente.
// La richiesta dell'antenna già associata si conferma da sola (serve a ritrovare il canale).
struct Richiesta {
  bool presente;
  uint8_t mac[6];
  bool tx;      // antenna TX (altrimenti RX)
  int8_t rssi;  // dBm
};

void inizia(const uint8_t peer[6]);
void aggiorna();
void invia(proto::Tipo tipo, uint32_t freq_hz = 0, int32_t valore = 0);
Stato stato();

// Chiede all'antenna tutti i punti di calibrazione della banda (uno alla volta).
void richiediTabella(bande::Banda banda);
Tabella tabella();

Richiesta richiestaAssociazione();
// Dal task di LVGL. Con conferma il chiamante salva il MAC nelle impostazioni; qui si risponde all'antenna
// e si passa al nuovo peer senza riavvio.
void rispondiAssociazione(bool conferma);

}  // namespace link_antenna
