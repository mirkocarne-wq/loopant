// Protocollo ESP-NOW tra unità di controllo (casa) e unità antenna.
// Documentazione: docs/software/04-protocollo-espnow.md
#pragma once
#include <stdint.h>

namespace proto {

// Versione 2: flag nella risposta, lettura ed eliminazione dei punti di calibrazione.
// Versione 3: comando IMPOSTA (driver del motore STEP/DIR o CAN), flag FLAG_CAN, errore MOTORE_CAN.
constexpr uint8_t VERSIONE = 3;

enum class Tipo : uint8_t {
  GOTO = 1,          // vai alla frequenza freq_hz
  STOP = 2,          // ferma il movimento
  ZERO = 3,          // azzeramento sul finecorsa
  STATO = 4,         // richiesta stato
  HEARTBEAT = 5,     // mantiene vivo il collegamento
  JOG = 6,           // movimento relativo di 'valore' passi
  CAL_SALVA = 7,     // associa la posizione attuale a freq_hz
  CAL_CANCELLA = 8,  // cancella la tabella della banda di freq_hz
  CAL_LEGGI = 9,     // chiede il punto numero 'valore' della banda di freq_hz
  CAL_ELIMINA = 10,  // elimina il punto più vicino a freq_hz (entro 1 kHz)
  IMPOSTA = 11,      // imposta il parametro freq_hz (Parametro) al valore 'valore'
  RISPOSTA = 0x80,      // antenna -> controllo: stato
  RISPOSTA_CAL = 0x81,  // antenna -> controllo: un punto di calibrazione
};

// Bit di Risposta::flag
constexpr uint8_t FLAG_FC_MIN = 1 << 0;      // finecorsa MIN premuto
constexpr uint8_t FLAG_FC_MAX = 1 << 1;      // finecorsa MAX premuto
constexpr uint8_t FLAG_DRIVER = 1 << 2;      // driver abilitato (motore in coppia)
constexpr uint8_t FLAG_VERSIONE_TX = 1 << 3; // antenna TX (altrimenti RX)
constexpr uint8_t FLAG_AZZERATA = 1 << 4;    // posizione nota
constexpr uint8_t FLAG_CAN = 1 << 5;         // motore pilotato via CAN (SERVO42D), altrimenti STEP/DIR

// Parametri del comando IMPOSTA (nel campo freq_hz)
enum class Parametro : uint32_t {
  MODO_MOTORE = 1,  // 0 = STEP/DIR, 1 = CAN (solo antenna RX con SERVO42D)
  CAN_ID = 2,       // indirizzo del SERVO42D, 1-2047
  CAN_KBIT = 3,     // velocità del bus: 125, 250, 500 o 1000
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
  MOTORE_CAN = 8,           // il SERVO42D non risponde sul bus CAN
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
  uint8_t flag;   // FLAG_*
};

// Risposta a CAL_LEGGI. totale = 0: banda senza punti (indice e valori non significativi).
struct __attribute__((packed)) RispostaCal {
  uint8_t versione;
  Tipo tipo;  // RISPOSTA_CAL
  uint16_t seq;
  uint16_t seq_comando;
  uint8_t banda;
  uint8_t indice;
  uint8_t totale;
  uint32_t freq_hz;
  int32_t posizione;
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
    case Errore::MOTORE_CAN: return "SERVO42D non risponde (CAN)";
  }
  return "?";
}

}  // namespace proto
