// Collegamento con la manopola remota (Waveshare ESP32-S3-Knob): riceve testo e stato del decoder, scatti della
// manopola (-> JOG all'antenna) e richieste di TUNE (-> GOTO); manda ogni secondo le impostazioni del decoder e i
// dati dell'antenna per il suo display. Il pannello resta l'unico a comandare l'antenna.
// aggiorna() va chiamata dal loop(); le altre funzioni anche dal task di LVGL.
#pragma once
#include <protocollo.h>
#include <stdint.h>

namespace link_manopola {

struct Stato {
  bool associata;     // MAC della manopola salvato
  bool collegata;     // stato ricevuto negli ultimi secondi
  int8_t rssi;
  proto::ManopolaStato s;  // ultimo stato ricevuto
};

struct Scelte {         // salvate in NVS ("manopola")
  uint8_t sorgente;     // proto::SorgenteAudio
  uint16_t passiScatto; // passi dell'antenna per scatto della manopola
};

void inizia();  // dopo link_antenna::inizia() (che accende ESP-NOW)
void aggiorna();
Stato stato();
Scelte& scelte();
void salvaScelte();  // e manda subito le impostazioni
void impostazioniCambiate();  // impostazioni del decoder cambiate: le manda subito

// Associazione (chiamate dal loop da link_antenna)
bool stessoMac(const uint8_t mac[6]);
void associa(const uint8_t mac[6]);  // salva il MAC e passa al nuovo peer
void dimentica();

}  // namespace link_manopola
