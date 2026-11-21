// Servizio QRZ.com: ricerca dei nominativi e lettura del logbook via HTTPS, in un task sul core 0.
// Le richieste partono dall'interfaccia o dalla console; i risultati si leggono con ricerca() e logbook().
#pragma once
#include <stdint.h>

#include "qrz.h"

namespace servizio_qrz {

enum class Esito : uint8_t {
  NESSUNO,            // nessuna richiesta ancora
  IN_CORSO,
  OK,
  NON_TROVATO,        // nominativo inesistente
  SENZA_CREDENZIALI,  // utente/password o chiave API mancanti
  SENZA_RETE,         // WiFi non collegato
  SERVE_ABBONAMENTO,  // logbook senza abbonamento XML Logbook Data
  ERRORE,
};

constexpr uint16_t MAX_QSO = 40;

struct Ricerca {
  Esito esito;
  uint32_t versione;  // cambia a ogni risposta
  qrz::Nominativo n;
  char messaggio[128];  // errore o avviso di QRZ (es. dati limitati senza abbonamento)
  // stato dell'accesso al servizio XML
  bool sessione;
  bool abbonato;
  char scadenza[40];
};

struct Logbook {
  Esito esito;
  uint32_t versione;
  uint8_t banda;  // filtro richiesto: 0 tutte, 40, 20
  uint16_t n;
  qrz::Qso qso[MAX_QSO];  // dal più recente
  uint32_t totale;
  uint32_t confermati;
  bool haConfermati;
  char messaggio[128];
};

void inizia();
void credenzialiCambiate();  // invalida la sessione: al prossimo uso nuovo accesso
void provaAccesso();         // accesso al servizio XML (per controllare utente e password)
void cerca(const char* nominativo);
void caricaQso(uint8_t banda);  // 0 tutte, 40, 20

uint32_t versioneRicerca();
uint32_t versioneLogbook();
void ricerca(Ricerca& out);
void logbook(Logbook& out);

const char* testo(Esito e);

}  // namespace servizio_qrz
