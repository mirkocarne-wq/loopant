// Protocollo ESP-NOW tra unità di controllo (casa) e unità antenna.
// Documentazione: docs/software/04-protocollo-espnow.md
#pragma once
#include <stdint.h>

namespace proto {

constexpr uint8_t VERSIONE = 1;

enum class Tipo : uint8_t {
  GOTO = 1,          // vai alla frequenza freq_hz
  STOP = 2,          // ferma il movimento
  ZERO = 3,          // azzeramento sul finecorsa
  STATO = 4,         // richiesta stato
  HEARTBEAT = 5,     // mantiene vivo il collegamento
  JOG = 6,           // movimento relativo di 'valore' passi
  CAL_SALVA = 7,     // associa la posizione attuale a freq_hz
  CAL_CANCELLA = 8,  // cancella la tabella della banda di freq_hz
  RISPOSTA = 0x80,   // antenna -> controllo
};

enum class Stato : uint8_t {
  NON_AZZERATA = 0,
  FERMO = 1,
  IN_MOVIMENTO = 2,
  AZZERAMENTO = 3,
  ERRORE = 4,
};

enum class Errore : uint8_t {
  NESSUNO = 0,
  FINECORSA = 1,            // finecorsa toccato fuori dall'azzeramento
  BLOCCO = 2,               // StallGuard
  FUORI_BANDA = 3,
  NON_CALIBRATA = 4,
  HEARTBEAT = 5,            // collegamento perso durante un movimento
  AZZERAMENTO_FALLITO = 6,  // finecorsa non trovato
  COMANDO_NON_VALIDO = 7,
};

struct __attribute__((packed)) Comando {
  uint8_t versione;
  Tipo tipo;
  uint16_t seq;
  uint32_t freq_hz;
  int32_t valore;
};

struct __attribute__((packed)) Risposta {
  uint8_t versione;
  Tipo tipo;
  uint16_t seq;
  uint16_t seq_comando;  // 0 = invio spontaneo
  Stato stato;
  Errore errore;
  uint8_t banda;  // 40, 20 oppure 0
  int32_t posizione;
  uint32_t freq_hz;
};

inline const char* nomeStato(Stato s) {
  switch (s) {
    case Stato::NON_AZZERATA: return "NON AZZERATA";
    case Stato::FERMO: return "FERMO";
    case Stato::IN_MOVIMENTO: return "IN MOVIMENTO";
    case Stato::AZZERAMENTO: return "AZZERAMENTO";
    case Stato::ERRORE: return "ERRORE";
  }
  return "?";
}

inline const char* nomeErrore(Errore e) {
  switch (e) {
    case Errore::NESSUNO: return "nessuno";
    case Errore::FINECORSA: return "finecorsa";
    case Errore::BLOCCO: return "blocco motore";
    case Errore::FUORI_BANDA: return "fuori banda";
    case Errore::NON_CALIBRATA: return "banda non calibrata";
    case Errore::HEARTBEAT: return "collegamento perso";
    case Errore::AZZERAMENTO_FALLITO: return "azzeramento fallito";
    case Errore::COMANDO_NON_VALIDO: return "comando non valido";
  }
  return "?";
}

}  // namespace proto
