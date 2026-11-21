#include "servizio_qrz.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <string.h>

#include "impostazioni.h"

// Certificati radice inclusi nel core (pacchetto completo di ESP-IDF): verifica vera del certificato di QRZ
extern const uint8_t certificati_inizio[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t certificati_fine[] asm("_binary_x509_crt_bundle_end");

namespace servizio_qrz {
namespace {

const char* URL_XML = "https://xmldata.qrz.com/xml/current/";
const char* URL_LOGBOOK = "https://logbook.qrz.com/api";
const char* AGENTE_HTTP = "loopant/0.1 (pannello mag loop, ESP32-S3)";  // richiesto dalla Logbook API
const char* AGENTE_XML = "loopant-0.1";

SemaphoreHandle_t mutex = nullptr;
TaskHandle_t compitoQrz = nullptr;

// Richieste in attesa
bool chiedeRicerca = false, chiedeLogbook = false, chiedeAccesso = false;
char nominativoChiesto[16];
uint8_t bandaChiesta = 0;
bool sessioneDaRifare = true;

// Stato pubblicato
Ricerca statoRicerca = {};
Logbook statoLogbook = {};
char chiaveSessione[64] = "";

// Area di lavoro del task (grande: fuori dallo stack)
Logbook lavoro;
uint32_t logid[MAX_QSO];

void blocca() { xSemaphoreTake(mutex, portMAX_DELAY); }
void sblocca() { xSemaphoreGive(mutex); }

void copia(char* out, size_t n, const char* s) {
  strncpy(out, s ? s : "", n - 1);
  out[n - 1] = 0;
}

String codifica(const char* s) {
  static char buf[600];
  qrz::codificaUrl(s, buf, sizeof(buf));
  return String(buf);
}

// POST HTTPS. true se HTTP 200. In ogni caso risposta contiene il corpo ricevuto (anche con 401/403) e codice lo stato.
bool post(const char* url, const String& corpo, String& risposta, char* errore, size_t ne, int* stato = nullptr) {
  risposta = String();
  if (stato) *stato = 0;
  if (WiFi.status() != WL_CONNECTED) {
    copia(errore, ne, "WiFi non collegato");
    return false;
  }
  NetworkClientSecure client;
  client.setCACertBundle(certificati_inizio, (size_t)(certificati_fine - certificati_inizio));
  HTTPClient http;
  http.setTimeout(15000);
  http.setUserAgent(AGENTE_HTTP);
  if (!http.begin(client, url)) {
    copia(errore, ne, "Connessione a QRZ non riuscita");
    return false;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int codice = http.POST(corpo);
  if (stato) *stato = codice;
  if (codice != 200) {
    if (codice < 0) snprintf(errore, ne, "QRZ non raggiungibile (%s)", http.errorToString(codice).c_str());
    else {
      snprintf(errore, ne, "QRZ: errore HTTP %d", codice);
      risposta = http.getString();
    }
    http.end();
    return false;
  }
  risposta = http.getString();
  http.end();
  return true;
}

void pubblicaRicerca(Esito e, const qrz::Nominativo* n, const char* messaggio) {
  blocca();
  statoRicerca.esito = e;
  if (n) statoRicerca.n = *n;
  copia(statoRicerca.messaggio, sizeof(statoRicerca.messaggio), messaggio);
  statoRicerca.versione++;
  sblocca();
}

// Accesso al servizio XML. Restituisce false (e pubblica l'esito) se non riesce.
bool accedi() {
  const ImpostazioniQrz& c = impostazioni::qrz();
  if (!c.utente[0] || !c.password[0]) {
    pubblicaRicerca(Esito::SENZA_CREDENZIALI, nullptr, "Inserisci utente e password QRZ in Impostazioni");
    return false;
  }
  String corpo = "username=" + codifica(c.utente) + "&password=" + codifica(c.password) + "&agent=" + AGENTE_XML;
  String r;
  char errore[96];
  if (!post(URL_XML, corpo, r, errore, sizeof(errore))) {
    pubblicaRicerca(WiFi.status() == WL_CONNECTED ? Esito::ERRORE : Esito::SENZA_RETE, nullptr, errore);
    return false;
  }
  qrz::Sessione s;
  qrz::analizzaXml(r.c_str(), s, nullptr);
  blocca();
  statoRicerca.sessione = s.valida;
  statoRicerca.abbonato = s.abbonato;
  copia(statoRicerca.scadenza, sizeof(statoRicerca.scadenza), s.scadenza);
  copia(chiaveSessione, sizeof(chiaveSessione), s.valida ? s.chiave : "");
  sblocca();
  if (!s.valida) {
    char m[128];
    snprintf(m, sizeof(m), "Accesso a QRZ non riuscito: %s", s.errore[0] ? s.errore : "risposta senza chiave");
    pubblicaRicerca(Esito::ERRORE, nullptr, m);
    return false;
  }
  sessioneDaRifare = false;
  return true;
}

void eseguiRicerca(const char* nominativo) {
  for (int tentativo = 0; tentativo < 2; tentativo++) {
    if (sessioneDaRifare && !accedi()) return;
    char chiave[64];
    blocca();
    copia(chiave, sizeof(chiave), chiaveSessione);
    sblocca();
    String corpo = "s=" + codifica(chiave) + "&callsign=" + codifica(nominativo);
    String r;
    char errore[96];
    if (!post(URL_XML, corpo, r, errore, sizeof(errore))) {
      pubblicaRicerca(WiFi.status() == WL_CONNECTED ? Esito::ERRORE : Esito::SENZA_RETE, nullptr, errore);
      return;
    }
    static qrz::Nominativo n;
    qrz::Sessione s;
    bool trovato = qrz::analizzaXml(r.c_str(), s, &n);
    if (!s.valida) {  // sessione scaduta: nuovo accesso e un secondo tentativo
      sessioneDaRifare = true;
      continue;
    }
    blocca();
    statoRicerca.sessione = true;
    statoRicerca.abbonato = s.abbonato;
    copia(statoRicerca.scadenza, sizeof(statoRicerca.scadenza), s.scadenza);
    copia(chiaveSessione, sizeof(chiaveSessione), s.chiave);
    sblocca();
    if (trovato) {
      pubblicaRicerca(Esito::OK, &n, s.messaggio);
    } else {
      char m[128];
      snprintf(m, sizeof(m), "%s", s.errore[0] ? s.errore : "Nominativo non trovato");
      pubblicaRicerca(Esito::NON_TROVATO, nullptr, m);
    }
    return;
  }
  pubblicaRicerca(Esito::ERRORE, nullptr, "Sessione QRZ non valida dopo un nuovo accesso");
}

// Una richiesta FETCH. false se non riesce (e lavoro.esito/messaggio spiegano perché).
bool fetch(const char* chiave, const String& opzione, qrz::Logbook& lb, qrz::Qso* qso, uint16_t max,
           uint16_t* letti, String* grezza) {
  String corpo = "KEY=" + codifica(chiave) + "&ACTION=FETCH&OPTION=" + codifica(opzione.c_str());
  String r;
  char errore[96];
  int stato = 0;
  bool ok = post(URL_LOGBOOK, corpo, r, errore, sizeof(errore), &stato);
  if (!ok && r.indexOf("RESULT=") < 0) {  // nessuna risposta della Logbook API da leggere
    lavoro.esito = WiFi.status() != WL_CONNECTED ? Esito::SENZA_RETE : Esito::ERRORE;
    if (stato == 401 || stato == 403)
      snprintf(lavoro.messaggio, sizeof(lavoro.messaggio),
               "Chiave API del logbook non valida o senza permessi (HTTP %d): controllala in Impostazioni", stato);
    else
      copia(lavoro.messaggio, sizeof(lavoro.messaggio), errore);
    return false;
  }
  uint16_t n = qrz::analizzaLogbook(r.c_str(), lb, qso, max);
  if (letti) *letti = n;
  if (strcmp(lb.result, "OK")) {
    bool abbonamento = qrz::serveAbbonamento(lb) && strcmp(lb.result, "AUTH") != 0;  // il motivo cita l'abbonamento
    if (abbonamento || (!strcmp(lb.result, "AUTH") && strstr(lb.reason, "ubscri"))) {
      lavoro.esito = Esito::SERVE_ABBONAMENTO;
      copia(lavoro.messaggio, sizeof(lavoro.messaggio),
            "La lettura del logbook via API richiede l'abbonamento QRZ «XML Logbook Data»");
    } else if (!strcmp(lb.result, "AUTH") && strstr(lb.reason, "invalid api key")) {
      lavoro.esito = Esito::ERRORE;
      copia(lavoro.messaggio, sizeof(lavoro.messaggio), "Chiave API del logbook non valida: controllala in Impostazioni");
    } else if (!strcmp(lb.result, "AUTH")) {  // RESULT=AUTH vale sia per chiave errata sia per permessi mancanti
      lavoro.esito = Esito::ERRORE;
      snprintf(lavoro.messaggio, sizeof(lavoro.messaggio),
               "QRZ ha negato l'accesso al logbook%s%s%s: controlla la chiave API (serve anche l'abbonamento "
               "«XML Logbook Data»)",
               lb.reason[0] ? " («" : "", lb.reason, lb.reason[0] ? "»)" : "");
    } else {
      lavoro.esito = Esito::ERRORE;
      snprintf(lavoro.messaggio, sizeof(lavoro.messaggio), "Logbook QRZ: %s",
               lb.reason[0] ? lb.reason : (lb.result[0] ? lb.result : "risposta non riconosciuta"));
    }
    return false;
  }
  if (grezza) *grezza = r;
  return true;
}

void eseguiLogbook(uint8_t banda) {
  memset(&lavoro, 0, sizeof(lavoro));
  lavoro.banda = banda;
  const ImpostazioniQrz& c = impostazioni::qrz();
  char filtro[16] = "";
  if (banda) snprintf(filtro, sizeof(filtro), ",BAND:%um", banda);
  qrz::Logbook lb;

  if (!c.chiave[0]) {
    lavoro.esito = Esito::SENZA_CREDENZIALI;
    copia(lavoro.messaggio, sizeof(lavoro.messaggio), "Inserisci la chiave API del logbook QRZ in Impostazioni");
  } else {
    // 1) elenco dei logid (leggero) per prendere gli ultimi MAX_QSO
    String ids;
    if (fetch(c.chiave, String("TYPE:LOGIDS") + filtro, lb, nullptr, 0, nullptr, &ids)) {
      lavoro.totale = lb.count;
      uint16_t k = qrz::ultimiLogid(ids.c_str(), logid, MAX_QSO);
      ids = String();
      bool ok = true;
      if (k) {
        // 2) i QSO di quei logid
        String opz = "LOGIDS:";
        for (uint16_t i = 0; i < k; i++) {
          if (i) opz += '+';
          opz += String(logid[i]);
        }
        uint16_t letti = 0;
        ok = fetch(c.chiave, opz, lb, lavoro.qso, MAX_QSO, &letti, nullptr);
        if (ok) {
          lavoro.n = letti;
          qrz::ordinaRecenti(lavoro.qso, letti);
        }
      }
      // 3) quanti confermati (solo il conteggio)
      if (ok && fetch(c.chiave, String("STATUS:CONFIRMED,MAX:0") + filtro, lb, nullptr, 0, nullptr, nullptr)) {
        lavoro.confermati = lb.count;
        lavoro.haConfermati = lb.haCount;
      }
      if (ok) {
        lavoro.esito = Esito::OK;
        lavoro.messaggio[0] = 0;
      }
    }
  }
  blocca();
  uint32_t v = statoLogbook.versione + 1;
  statoLogbook = lavoro;
  statoLogbook.versione = v;
  sblocca();
}

void compito(void*) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
    bool r, l, a;
    char nom[16];
    uint8_t banda;
    blocca();
    r = chiedeRicerca;
    l = chiedeLogbook;
    a = chiedeAccesso;
    chiedeRicerca = chiedeLogbook = chiedeAccesso = false;
    copia(nom, sizeof(nom), nominativoChiesto);
    banda = bandaChiesta;
    sblocca();
    if (a) {
      sessioneDaRifare = true;
      if (accedi()) {
        blocca();
        bool abb = statoRicerca.abbonato;
        sblocca();
        pubblicaRicerca(Esito::NESSUNO, nullptr,
                        abb ? "Accesso a QRZ riuscito" : "Accesso a QRZ riuscito, senza abbonamento: dati limitati");
      }
    }
    if (r) eseguiRicerca(nom);
    if (l) eseguiLogbook(banda);
  }
}

void sveglia() {
  if (compitoQrz) xTaskNotifyGive(compitoQrz);
}

}  // namespace

void inizia() {
  if (compitoQrz) return;
  mutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(compito, "qrz", 12288, nullptr, 1, &compitoQrz, 0);  // TLS: stack abbondante
}

void credenzialiCambiate() {
  blocca();
  sessioneDaRifare = true;
  statoRicerca.sessione = false;
  sblocca();
}

void provaAccesso() {
  blocca();
  chiedeAccesso = true;
  statoRicerca.esito = Esito::IN_CORSO;
  statoRicerca.versione++;
  sblocca();
  sveglia();
}

void cerca(const char* nominativo) {
  blocca();
  copia(nominativoChiesto, sizeof(nominativoChiesto), nominativo);
  for (char* c = nominativoChiesto; *c; c++) *c = (char)toupper((unsigned char)*c);
  chiedeRicerca = true;
  statoRicerca.esito = Esito::IN_CORSO;
  statoRicerca.versione++;
  sblocca();
  sveglia();
}

void caricaQso(uint8_t banda) {
  blocca();
  bandaChiesta = banda;
  chiedeLogbook = true;
  statoLogbook.esito = Esito::IN_CORSO;
  statoLogbook.banda = banda;
  statoLogbook.versione++;
  sblocca();
  sveglia();
}

uint32_t versioneRicerca() {
  blocca();
  uint32_t v = statoRicerca.versione;
  sblocca();
  return v;
}

uint32_t versioneLogbook() {
  blocca();
  uint32_t v = statoLogbook.versione;
  sblocca();
  return v;
}

void ricerca(Ricerca& out) {
  blocca();
  out = statoRicerca;
  sblocca();
}

void logbook(Logbook& out) {
  blocca();
  out = statoLogbook;
  sblocca();
}

const char* testo(Esito e) {
  switch (e) {
    case Esito::NESSUNO: return "";
    case Esito::IN_CORSO: return "Richiesta a QRZ in corso…";
    case Esito::OK: return "";
    case Esito::NON_TROVATO: return "Nominativo non trovato";
    case Esito::SENZA_CREDENZIALI: return "Credenziali QRZ mancanti";
    case Esito::SENZA_RETE: return "Serve il WiFi (Impostazioni)";
    case Esito::SERVE_ABBONAMENTO: return "Serve l'abbonamento QRZ";
    case Esito::ERRORE: return "Errore";
  }
  return "";
}

}  // namespace servizio_qrz
