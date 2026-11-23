// Stato condiviso della manopola tra il loop (collegamento con il pannello) e il task di LVGL (interfaccia).
// Si legge e si scrive per copia, con una sezione critica.
#pragma once
#include <protocollo.h>
#include <stdint.h>

namespace condiviso {

struct Pannello {         // scritto dal collegamento
  bool collegato;         // impostazioni ricevute negli ultimi secondi
  bool associato;         // MAC del pannello salvato
  int8_t rssi;            // dBm
  uint8_t associazione;   // associazione::Fase
  uint8_t canale;
  bool valida;            // almeno un ManopolaImposta ricevuto
  proto::ManopolaImposta imp;
  char macPannello[18];   // pannello associato ("" se nessuno)
};

struct Comandi {          // scritto dall'interfaccia, mandato al pannello
  bool modoFrequenza;     // false = ritocco fine
  int32_t scatti;         // scatti nel ritocco fine (contatore)
  uint32_t freqModifica;  // frequenza in modifica (modo Frequenza)
  uint16_t nTune;
  uint32_t freqTune;
  uint16_t nZero;
  uint16_t nDecoder;      // richiesta di accendere/spegnere il decoder (dal menu)
  uint8_t decoderVoluto;
  uint16_t nAssocia;      // richiesta di associazione a un pannello (dal menu)
  uint16_t nDimentica;    // richiesta di dimenticare il pannello (dal menu)
};

Pannello pannello();
void pannello(const Pannello& p);
Comandi comandi();
void comandi(const Comandi& c);

}  // namespace condiviso
