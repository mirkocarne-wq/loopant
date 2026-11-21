// Collegamento ESP-NOW cifrato: un'istanza per ogni peer (il pannello ne ha due: antenna e nodo audio).
// La prima istanza avviata accende WiFi ed ESP-NOW; le altre aggiungono solo il loro peer.
// I pacchetti ricevuti finiscono nella coda dell'istanza del mittente e si leggono dal loop con ricevi().
#pragma once
#include <Arduino.h>

struct Pacchetto {
  uint8_t len;
  int8_t rssi;  // dBm; 0 se non disponibile (core Arduino 2.x)
  uint8_t dati[250];  // massimo di ESP-NOW
};

// Pacchetto in broadcast da un'unità qualsiasi (associazione)
struct Annuncio {
  uint8_t mac[6];
  uint8_t len;
  int8_t rssi;
  uint8_t dati[32];
};

class Collegamento {
 public:
  // peer: MAC dell'altra unità (zeri = da associare); pmk/lmk: chiavi da 16 byte; canale: canale WiFi comune
  // (usato solo dalla prima istanza).
  bool inizia(const uint8_t peer[6], const uint8_t pmk[16], const uint8_t lmk[16], uint8_t canale);
  bool invia(const void* dati, size_t len);
  // Invio al peer con attesa della conferma di ricezione della sua radio (ACK): true se il peer l'ha ricevuto.
  // Serve per trovare il canale del pannello. Blocca al massimo attesaMs.
  bool inviaConConferma(const void* dati, size_t len, uint32_t attesaMs);
  bool ricevi(Pacchetto& p);
  bool peerValido() const { return peerOk_; }
  const uint8_t* macPeer() const { return macPeer_; }
  bool cambiaPeer(const uint8_t peer[6], const uint8_t lmk[16]);  // associazione, senza riavvio

  // Associazione: pacchetti in chiaro a tutti e loro ricezione (comuni a tutte le istanze).
  static bool inviaATutti(const void* dati, size_t len);
  static bool riceviAnnuncio(Annuncio& a);

  static bool stessoMac(const uint8_t a[6], const uint8_t b[6]);
  static void macLocale(uint8_t out[6]);
  static bool leggiMac(const char* testo, uint8_t out[6]);
  static void scriviMac(const uint8_t mac[6], char out[18]);

  // Uso interno (callback di ESP-NOW)
  static constexpr int MAX_ISTANZE = 3;
  static Collegamento* istanze[MAX_ISTANZE];
  QueueHandle_t coda_ = nullptr;
  uint8_t macPeer_[6] = {0};
  bool peerOk_ = false;
};
