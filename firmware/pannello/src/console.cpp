#include "console.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <collegamento.h>
#include <ctype.h>
#include <string.h>

#include "impostazioni.h"
#include "link_antenna.h"
#include "ora.h"
#include "rete.h"
#include "servizio_qrz.h"
#include "tema.h"
#include "ui/ui.h"

namespace console {
namespace {

// La console funziona su entrambe le USB-C del 7B: USB nativa (Serial, con "USB CDC On Boot") e
// "USB TO UART" (CH343, Serial0). Si scrive su tutte e due.
class UscitaDoppia : public Print {
 public:
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t* b, size_t n) override {
#if ARDUINO_USB_CDC_ON_BOOT
    Serial0.write(b, n);
#endif
    return Serial.write(b, n);
  }
};
UscitaDoppia uscita;

// Risposte di QRZ da stampare quando arrivano
uint32_t attesaRicerca = 0, attesaLogbook = 0;  // versione al momento della richiesta (0 = niente)

uint32_t leggiFrequenza(const char* s) {
  double v = atof(s);
  if (v <= 0) return 0;
  return v > 100000 ? (uint32_t)v : (uint32_t)(v * 1000.0 + 0.5);  // kHz se < 100000
}

void copiaMaiuscolo(char* dst, size_t n, const char* src) {
  size_t i = 0;
  for (; src[i] && i < n - 1; i++) dst[i] = (char)toupper((unsigned char)src[i]);
  dst[i] = 0;
}

void aiuto() {
  uscita.println(
      "Comandi: goto <kHz> | stop | zero | jog <passi> | cal <kHz> | calcanc <kHz> | stato |\n"
      "         mac <AA:BB:CC:DD:EE:FF> | call <nominativo> | loc <locator> | tema <0-3> |\n"
      "         ora <AAAA-MM-GG HH:MM> (UTC) | wifi <ssid> <password> | reti | rete |\n"
      "         qrz <utente> <password> | qrzkey <chiave> | qrzcancella | cerca <nominativo> | qso [40|20] |\n"
      "         mem | aiuto");
}

void stato() {
  link_antenna::Stato s = link_antenna::stato();
  uscita.printf("Collegamento: %s, ultima risposta %lu ms fa, comandi ripetuti %lu/%lu\n",
                s.collegato ? "attivo" : "assente", (unsigned long)s.etaMs, (unsigned long)s.ripetuti,
                (unsigned long)s.inviati);
  if (s.ricevuta)
    uscita.printf("Antenna: %s, errore: %s, posizione %ld, freq %.1f kHz\n", proto::nomeStato(s.r.stato),
                  proto::nomeErrore(s.r.errore), (long)s.r.posizione, s.r.freq_hz / 1000.0);
}

void impostaOra(const char* data, const char* hm) {
  int a, m, g, h, mi;
  if (!data || !hm || sscanf(data, "%d-%d-%d", &a, &m, &g) != 3 || sscanf(hm, "%d:%d", &h, &mi) != 2 ||
      !ora::impostaUtc(a, m, g, h, mi)) {
    uscita.println("Formato: ora AAAA-MM-GG HH:MM (UTC)");
    return;
  }
  uscita.println("Ora impostata (UTC)");
}

void esegui(char* riga) {
  char* cmd = strtok(riga, " \t");
  char* a1 = strtok(nullptr, " \t");
  char* a2 = strtok(nullptr, " \t");
  if (!cmd) return;
  Impostazioni& imp = impostazioni::get();

  if (!strcmp(cmd, "goto") && a1) link_antenna::invia(proto::Tipo::GOTO, leggiFrequenza(a1));
  else if (!strcmp(cmd, "stop")) link_antenna::invia(proto::Tipo::STOP);
  else if (!strcmp(cmd, "zero")) link_antenna::invia(proto::Tipo::ZERO);
  else if (!strcmp(cmd, "jog") && a1) link_antenna::invia(proto::Tipo::JOG, 0, atol(a1));
  else if (!strcmp(cmd, "cal") && a1) link_antenna::invia(proto::Tipo::CAL_SALVA, leggiFrequenza(a1));
  else if (!strcmp(cmd, "calcanc") && a1) link_antenna::invia(proto::Tipo::CAL_CANCELLA, leggiFrequenza(a1));
  else if (!strcmp(cmd, "stato")) stato();
  else if (!strcmp(cmd, "mac") && a1) {
    if (!Collegamento::leggiMac(a1, imp.peer)) {
      uscita.println("MAC non valido");
      return;
    }
    impostazioni::salva();
    uscita.println("MAC dell'antenna salvato: riavviare");
  } else if (!strcmp(cmd, "call") && a1) {
    copiaMaiuscolo(imp.nominativo, sizeof(imp.nominativo), a1);
    impostazioni::salva();
    ui::richiediRicostruzione();
  } else if (!strcmp(cmd, "loc") && a1) {
    copiaMaiuscolo(imp.locator, sizeof(imp.locator), a1);
    impostazioni::salva();
    ui::richiediRicostruzione();
  } else if (!strcmp(cmd, "tema") && a1) {
    imp.tema = (uint8_t)atoi(a1) % tema::NUMERO;
    impostazioni::salva();
    ui::richiediRicostruzione();
    uscita.printf("Tema: %s\n", tema::di(imp.tema).nome);
  } else if (!strcmp(cmd, "ora")) impostaOra(a1, a2);
  else if (!strcmp(cmd, "wifi") && a1) {
    strncpy(imp.wifiSsid, a1, sizeof(imp.wifiSsid) - 1);
    imp.wifiSsid[sizeof(imp.wifiSsid) - 1] = 0;
    strncpy(imp.wifiPassword, a2 ? a2 : "", sizeof(imp.wifiPassword) - 1);
    imp.wifiPassword[sizeof(imp.wifiPassword) - 1] = 0;
    impostazioni::salva();
    rete::riavvia();
    uscita.println("WiFi salvato: connessione in corso");
  } else if (!strcmp(cmd, "reti")) {
    rete::Rete r[rete::MAX_RETI];
    bool inCorso;
    uint32_t v;
    uint8_t n = rete::reti(r, rete::MAX_RETI, inCorso, v);
    if (inCorso) {
      uscita.println("Ricerca in corso: ripeti 'reti' tra qualche secondo");
      return;
    }
    for (uint8_t i = 0; i < n; i++)
      uscita.printf("%2u  %4d dBm  can %2u  %-8s  %s\n", i + 1, r[i].rssi, r[i].canale,
                    r[i].protetta ? "protetta" : "aperta", r[i].ssid);
    if (!n && v) uscita.println("Nessuna rete trovata");
    rete::avviaScansione();
    uscita.println(v ? "Nuova ricerca avviata" : "Ricerca avviata: ripeti 'reti' tra qualche secondo");
  } else if (!strcmp(cmd, "rete")) {
    char t[96];
    rete::stato(t, sizeof(t));
    uscita.println(t);
  } else if (!strcmp(cmd, "qrz") && a1 && a2) {
    ImpostazioniQrz& q = impostazioni::qrz();
    copiaMaiuscolo(q.utente, sizeof(q.utente), a1);
    strncpy(q.password, a2, sizeof(q.password) - 1);
    q.password[sizeof(q.password) - 1] = 0;
    impostazioni::salvaQrz();
    servizio_qrz::credenzialiCambiate();
    uscita.println("Credenziali QRZ salvate");
  } else if (!strcmp(cmd, "qrzkey") && a1) {
    ImpostazioniQrz& q = impostazioni::qrz();
    copiaMaiuscolo(q.chiave, sizeof(q.chiave), a1);
    impostazioni::salvaQrz();
    uscita.println("Chiave API del logbook salvata");
  } else if (!strcmp(cmd, "mem")) {
    uscita.printf("RAM interna: libera %u, minimo %u, blocco più grande %u · PSRAM libera %u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  } else if (!strcmp(cmd, "qrzcancella")) {
    ImpostazioniQrz& q = impostazioni::qrz();
    q.utente[0] = q.password[0] = q.chiave[0] = 0;
    impostazioni::salvaQrz();
    servizio_qrz::credenzialiCambiate();
    uscita.println("Credenziali QRZ cancellate");
  } else if (!strcmp(cmd, "cerca") && a1) {
    servizio_qrz::cerca(a1);
    attesaRicerca = servizio_qrz::versioneRicerca();
  } else if (!strcmp(cmd, "qso")) {
    servizio_qrz::caricaQso(a1 ? (uint8_t)atoi(a1) : 0);
    attesaLogbook = servizio_qrz::versioneLogbook();
  }
  else aiuto();
}

}  // namespace

void inizia() {
  uint8_t mio[6];
  char testo[18];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, testo);
  uscita.printf("\nloopant - pannello in casa\nMAC di questo pannello: %s\n", testo);
  if (!link_antenna::stato().peerImpostato)
    uscita.println("MAC dell'antenna non impostato: usare 'mac AA:BB:CC:DD:EE:FF'");
  aiuto();
}

void stampaQrz() {
  using servizio_qrz::Esito;
  if (attesaRicerca && servizio_qrz::versioneRicerca() != attesaRicerca) {
    static servizio_qrz::Ricerca r;
    servizio_qrz::ricerca(r);
    if (r.esito != Esito::IN_CORSO) {
      attesaRicerca = 0;
      if (r.esito == Esito::OK) {
        const qrz::Nominativo& n = r.n;
        uscita.printf("%s · %s\n  QTH %s · %s (DXCC %d) · locator %s · CQ %d ITU %d\n  QSL %s\n", n.call, n.nome,
                      n.qth, n.paese, n.dxcc, n.locator, n.cq, n.itu, n.qsl);
        uscita.printf("  %s%s%s\n", r.abbonato ? "abbonamento fino a " : "senza abbonamento", r.abbonato ? r.scadenza : "",
                      r.messaggio[0] ? " · " : "");
        if (r.messaggio[0]) uscita.println(r.messaggio);
      } else {
        uscita.printf("QRZ: %s%s%s\n", servizio_qrz::testo(r.esito), r.messaggio[0] ? ": " : "", r.messaggio);
      }
    } else {
      attesaRicerca = servizio_qrz::versioneRicerca();
    }
  }
  if (attesaLogbook && servizio_qrz::versioneLogbook() != attesaLogbook) {
    static servizio_qrz::Logbook l;
    servizio_qrz::logbook(l);
    if (l.esito != Esito::IN_CORSO) {
      attesaLogbook = 0;
      if (l.esito == Esito::OK) {
        uscita.printf("Logbook: %lu QSO", (unsigned long)l.totale);
        if (l.haConfermati) uscita.printf(", %lu confermati", (unsigned long)l.confermati);
        uscita.printf(" · ultimi %u:\n", l.n);
        for (uint16_t i = 0; i < l.n && i < 15; i++)
          uscita.printf("  %s %s  %-10s %-5s %-6s %9.3f MHz %s\n", l.qso[i].data, l.qso[i].ora, l.qso[i].call,
                        l.qso[i].banda, l.qso[i].modo, l.qso[i].freq_hz / 1e6, l.qso[i].confermato ? "confermato" : "");
      } else {
        uscita.printf("Logbook QRZ: %s%s%s\n", servizio_qrz::testo(l.esito), l.messaggio[0] ? ": " : "", l.messaggio);
      }
    } else {
      attesaLogbook = servizio_qrz::versioneLogbook();
    }
  }
}

struct Riga {
  char testo[160];
  size_t n;
};

void leggi(Stream& porta, Riga& r) {
  while (porta.available()) {
    char c = (char)porta.read();
    if (c == '\r') continue;
    if (c == '\n') {
      r.testo[r.n] = 0;
      r.n = 0;
      esegui(r.testo);
    } else if (r.n < sizeof(r.testo) - 1) {
      r.testo[r.n++] = c;
    }
  }
}

void aggiorna() {
  stampaQrz();
  static Riga usb = {}, uart = {};
  leggi(Serial, usb);
#if ARDUINO_USB_CDC_ON_BOOT
  leggi(Serial0, uart);
#else
  (void)uart;
#endif
}

}  // namespace console
