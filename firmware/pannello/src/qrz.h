// QRZ.com: analisi delle risposte dei due servizi (C++ puro, provato sul PC con test/test_qrz.cpp).
// - XML Data: ricerca dei nominativi (https://xmldata.qrz.com/xml/current/)
// - Logbook API: i QSO del proprio logbook (https://logbook.qrz.com/api)
// Specifiche e limiti dell'abbonamento: docs/software/10-qrz.md
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace qrz {

struct Sessione {
  bool valida;          // la risposta contiene una chiave di sessione
  char chiave[64];
  bool abbonato;        // SubExp diverso da "non-subscriber"
  char scadenza[40];    // testo di SubExp
  char errore[96];      // es. "Not found: XX1XX", "Session Timeout", "password incorrect"
  char messaggio[128];  // es. "A subscription is required to obtain the complete data"
};

struct Nominativo {
  char call[16];
  char nome[64];
  char qth[64];  // città, stato
  char paese[40];
  char locator[8];
  bool coordinate;
  double lat, lon;
  int dxcc, cq, itu;  // 0 = sconosciuto
  char qsl[72];       // "LoTW · eQSL · carta · via MGR"
  char classe[16];
  char immagine[160];
};

// Analizza una risposta XML. Riempie sempre la sessione; restituisce true se c'è un nominativo (in n, se non nullo).
bool analizzaXml(const char* xml, Sessione& s, Nominativo* n);

struct Qso {
  uint32_t logid;
  char call[16];
  char data[11];    // "20/11/2026"
  char ora[6];      // "14:32" (UTC)
  char ordine[15];  // "20261120143200": per ordinare
  char banda[8];
  char modo[10];
  uint32_t freq_hz;  // 0 se assente
  char rstTx[6], rstRx[6];
  bool confermato;
};

struct Logbook {
  char result[12];   // OK, FAIL, AUTH...
  char reason[128];
  uint32_t count;
  bool haCount;
};

// Risposta della Logbook API: RESULT, REASON, COUNT e, se c'è, l'ADIF (con o senza codifica URL o entità HTML).
// qso può essere nullo. Restituisce quanti QSO sono stati letti (al massimo max).
uint16_t analizzaLogbook(const char* risposta, Logbook& lb, Qso* qso, uint16_t max);

// Dalla risposta di FETCH con TYPE:LOGIDS: gli n logid più alti (i più recenti), in ordine decrescente.
uint16_t ultimiLogid(const char* risposta, uint32_t* out, uint16_t n);

// Codifica per application/x-www-form-urlencoded. Restituisce false se out è troppo piccolo.
bool codificaUrl(const char* in, char* out, size_t n);

// Ordina dal QSO più recente.
void ordinaRecenti(Qso* q, uint16_t n);

// true se la risposta del logbook indica che serve l'abbonamento (RESULT=AUTH o motivo che lo cita).
bool serveAbbonamento(const Logbook& lb);

}  // namespace qrz
