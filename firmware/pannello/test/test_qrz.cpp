// Test sul PC della lettura delle risposte QRZ (dati inventati) e dei calcoli di distanza/direzione.
// g++ -std=c++17 -I../src test_qrz.cpp ../src/qrz.cpp ../src/geo.cpp -o test_qrz && ./test_qrz
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "geo.h"
#include "qrz.h"

static int errori = 0;
static void verifica(bool ok, const char* cosa) {
  printf("%s %s\n", ok ? "OK  " : "ERRORE", cosa);
  if (!ok) errori++;
}

// Risposta di ricerca con abbonamento (struttura come nella specifica QRZ, dati inventati)
static const char* XML_COMPLETO =
    "<?xml version=\"1.0\" ?>\n<QRZDatabase version=\"1.34\" xmlns=\"http://xmldata.qrz.com\">\n"
    "<Callsign>\n<call>XX1ABC</call>\n<dxcc>291</dxcc>\n<fname>MARIO &amp; CO</fname>\n<name>ROSSI</name>\n"
    "<addr2>NEW YORK</addr2>\n<state>NY</state>\n<country>United States</country>\n"
    "<lat>40.7128</lat>\n<lon>-74.0060</lon>\n<grid>FN30as</grid>\n<class>E</class>\n"
    "<qslmgr>NONE</qslmgr>\n<lotw>1</lotw>\n<eqsl>Y</eqsl>\n<mqsl>N</mqsl>\n<cqzone>5</cqzone>\n<ituzone>8</ituzone>\n"
    "<image>https://files.qrz.com/x/xx1abc/foto.jpg</image>\n</Callsign>\n"
    "<Session>\n<Key>abc123def456</Key>\n<Count>42</Count>\n<SubExp>Wed Jan 1 12:34:03 2027</SubExp>\n"
    "<GMTime>Sat Nov 21 10:00:00 2026</GMTime>\n</Session>\n</QRZDatabase>\n";

// Senza abbonamento: pochi campi e un messaggio
static const char* XML_LIMITATO =
    "<?xml version=\"1.0\" ?><QRZDatabase version=\"1.34\"><Callsign><call>XX2DEF</call><fname>ANNA</fname>"
    "<name>BIANCHI</name><addr2>ROMA</addr2><country>Italy</country></Callsign><Session><Key>k9</Key>"
    "<SubExp>non-subscriber</SubExp><Message>A subscription is required to obtain the complete data.</Message>"
    "</Session></QRZDatabase>";

static const char* XML_NON_TROVATO =
    "<?xml version=\"1.0\" ?><QRZDatabase version=\"1.34\"><Session><Error>Not found: XX9ZZZ</Error>"
    "<Key>k9</Key><SubExp>non-subscriber</SubExp></Session></QRZDatabase>";

static const char* XML_SCADUTA =
    "<?xml version=\"1.0\" ?><QRZDatabase version=\"1.34\"><Session><Error>Session Timeout</Error>"
    "</Session></QRZDatabase>";

// Logbook: ADIF con entità HTML (due QSO, il secondo confermato)
static const char* LOG_ENTITA =
    "RESULT=OK&COUNT=2&ADIF=&lt;band:3&gt;20m&lt;mode:3&gt;FT8&lt;call:6&gt;XX1ABC&lt;qso_date:8&gt;20261120"
    "&lt;time_on:6&gt;143215&lt;freq:6&gt;14.074&lt;rst_sent:3&gt;-10&lt;rst_rcvd:3&gt;-15"
    "&lt;app_qrzlog_logid:4&gt;1001&lt;eor&gt;"
    "&lt;band:3&gt;40M&lt;mode:2&gt;CW&lt;call:6&gt;XX2D&amp;F&lt;qso_date:8&gt;20261119&lt;time_on:4&gt;2100"
    "&lt;freq:5&gt;7.030&lt;rst_sent:3&gt;599&lt;rst_rcvd:3&gt;579&lt;lotw_qsl_rcvd:1&gt;Y"
    "&lt;app_qrzlog_logid:4&gt;1000&lt;eor&gt;";

// Lo stesso con codifica URL e i campi in un altro ordine
static const char* LOG_URL =
    "COUNT=2&ADIF=%3Cband%3A3%3E20m%3Cmode%3A4%3EMFSK%3Csubmode%3A3%3EFT4%3Ccall%3A6%3EXX1ABC"
    "%3Cqso_date%3A8%3E20261120%3Ctime_on%3A4%3E1432%3Cfreq%3A6%3E14.080%3Capp_qrzlog_status%3A1%3EC%3Ceor%3E"
    "%3Cband%3A3%3E40m%3Cmode%3A3%3ESSB%3Ccall%3A6%3EXX2DEF%3Cqso_date%3A8%3E20261121%3Ctime_on%3A4%3E0715"
    "%3Cfreq%3A5%3E7.090%3Ceor%3E&RESULT=OK";

static const char* LOG_AUTH = "RESULT=AUTH&REASON=subscription+required";
// Risposta vera di QRZ con una chiave inventata (21/11/2026)
static const char* LOG_CHIAVE =
    "RESULT=AUTH&REASON=API+USER%3A++BOOK%3A++STATUS%3A+AUTH+REASON%3A+invalid+api+key+EXTENDED%3A+&EXTENDED=";
static const char* LOG_IDS = "RESULT=OK&COUNT=6&LOGIDS=1005%2C1001%2C1009%2C1002%2C1012%2C1003";

int main() {
  qrz::Sessione s;
  qrz::Nominativo n;

  verifica(qrz::analizzaXml(XML_COMPLETO, s, &n), "XML completo: nominativo trovato");
  verifica(s.valida && !strcmp(s.chiave, "abc123def456"), "chiave di sessione");
  verifica(s.abbonato, "abbonamento riconosciuto");
  verifica(!strcmp(n.call, "XX1ABC") && !strcmp(n.nome, "MARIO & CO ROSSI"), "nominativo e nome (con &amp;)");
  verifica(!strcmp(n.qth, "NEW YORK, NY") && !strcmp(n.paese, "United States"), "QTH e paese");
  verifica(n.coordinate && fabs(n.lat - 40.7128) < 1e-6 && !strcmp(n.locator, "FN30as"), "coordinate e locator");
  verifica(n.dxcc == 291 && n.cq == 5 && n.itu == 8, "DXCC e zone");
  printf("     QSL: \"%s\"\n", n.qsl);
  verifica(!strcmp(n.qsl, "LoTW · eQSL"), "QSL: LoTW ed eQSL, niente carta né manager NONE");

  verifica(qrz::analizzaXml(XML_LIMITATO, s, &n) && !s.abbonato && s.valida, "senza abbonamento: dati e chiave");
  verifica(!strcmp(n.nome, "ANNA BIANCHI") && !n.coordinate && !n.locator[0], "senza abbonamento: campi limitati");
  verifica(strstr(s.messaggio, "subscription") != nullptr, "messaggio sull'abbonamento");

  verifica(!qrz::analizzaXml(XML_NON_TROVATO, s, &n) && s.valida && !strcmp(s.errore, "Not found: XX9ZZZ"),
           "nominativo non trovato, sessione ancora valida");
  verifica(!qrz::analizzaXml(XML_SCADUTA, s, &n) && !s.valida, "sessione scaduta: niente chiave");

  qrz::Logbook lb;
  qrz::Qso q[8];
  uint16_t k = qrz::analizzaLogbook(LOG_ENTITA, lb, q, 8);
  verifica(k == 2 && !strcmp(lb.result, "OK") && lb.count == 2, "logbook con entità: 2 QSO");
  verifica(!strcmp(q[0].call, "XX1ABC") && !strcmp(q[0].data, "20/11/2026") && !strcmp(q[0].ora, "14:32") &&
               q[0].freq_hz == 14074000 && !strcmp(q[0].modo, "FT8") && !q[0].confermato,
           "primo QSO: data, ora, frequenza, modo");
  verifica(!strcmp(q[1].call, "XX2D&F") && !strcmp(q[1].banda, "40m") && q[1].confermato &&
               !strcmp(q[1].rstTx, "599") && q[1].logid == 1000,
           "secondo QSO: entità, banda in minuscolo, confermato LoTW");

  k = qrz::analizzaLogbook(LOG_URL, lb, q, 8);
  verifica(k == 2 && !strcmp(lb.result, "OK"), "logbook con codifica URL e campi in altro ordine");
  verifica(!strcmp(q[0].modo, "FT4") && q[0].confermato, "submode FT4 e stato QRZ confermato");
  qrz::ordinaRecenti(q, k);
  verifica(!strcmp(q[0].call, "XX2DEF") && !strcmp(q[0].ora, "07:15"), "ordinamento dal più recente");

  k = qrz::analizzaLogbook(LOG_AUTH, lb, q, 8);
  verifica(k == 0 && qrz::serveAbbonamento(lb), "RESULT=AUTH: serve l'abbonamento");

  qrz::analizzaLogbook(LOG_CHIAVE, lb, q, 8);
  printf("     motivo: \"%s\"\n", lb.reason);
  verifica(!strcmp(lb.result, "AUTH") && !strcmp(lb.reason, "invalid api key"),
           "chiave non valida: si estrae il motivo");

  uint32_t ids[4];
  k = qrz::ultimiLogid(LOG_IDS, ids, 4);
  verifica(k == 4 && ids[0] == 1012 && ids[1] == 1009 && ids[2] == 1005 && ids[3] == 1003, "ultimi 4 logid");

  char buf[64];
  verifica(qrz::codificaUrl("LOGIDS:1012+1009,BAND:40m", buf, sizeof(buf)) &&
               !strcmp(buf, "LOGIDS%3A1012%2B1009%2CBAND%3A40m"),
           "codifica URL (il + diventa %2B)");

  // Londra -> New York: circa 5570 km, direzione iniziale circa 288°
  double d = geo::distanzaKm(51.5074, -0.1278, 40.7128, -74.0060);
  double az = geo::azimut(51.5074, -0.1278, 40.7128, -74.0060);
  printf("     Londra -> New York: %.0f km, %.1f°\n", d, az);
  verifica(fabs(d - 5570) < 15 && fabs(az - 288.3) < 1, "distanza e direzione Londra -> New York");
  double la, lo;
  geo::intermedio(0, 0, 0, 90, 0.5, la, lo);
  verifica(fabs(la) < 1e-9 && fabs(lo - 45) < 1e-9, "punto intermedio sull'equatore");

  printf(errori ? "\n%d ERRORI\n" : "\nTutti i test superati\n", errori);
  return errori ? 1 : 0;
}
