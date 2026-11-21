#include "link_antenna.h"

#include <Arduino.h>
#include <collegamento.h>
#include <segreti_scelti.h>
#include <string.h>

#include "config.h"

namespace link_antenna {
namespace {

Collegamento radio;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

uint16_t seq = 0;
uint32_t ultimoInvio = 0;

bool inAttesa = false;
proto::Comando attesa = {};
uint8_t tentativi = 0;
uint32_t istanteInvio = 0;

bool ricevuta = false;
bool fallito = false;
proto::Risposta ultima = {};
uint32_t istanteRisposta = 0;
int8_t rssi = 0;
uint32_t ripetuti = 0, inviati = 0, ricevuti = 0;

Tabella tab = {};

// comandi in coda mentre se ne attende la risposta di un altro
constexpr uint8_t MAX_CODA = 6;
proto::Comando coda[MAX_CODA];
uint8_t nCoda = 0;
bool letturaInCorso = false;
uint8_t prossimoIndice = 0;
bool chiediProssimo = false;

uint16_t prossimaSeq() {
  if (++seq == 0) seq = 1;  // 0 è riservato alle risposte spontanee
  return seq;
}

void trasmetti(const proto::Comando& c) {
  radio.invia(&c, sizeof(c));
  ultimoInvio = millis();
}

uint32_t freqDiBanda(bande::Banda b) {
  const bande::Info* i = bande::info(b);
  return i ? i->min_hz : 0;
}

void riceviStato(const proto::Risposta& r) {
  ultima = r;
  if (inAttesa && r.seq_comando == attesa.seq) inAttesa = false;
}

void riceviPunto(const proto::RispostaCal& r) {
  if (inAttesa && r.seq_comando == attesa.seq) inAttesa = false;
  if (!letturaInCorso || r.banda != (uint8_t)tab.banda) return;
  tab.totale = r.totale > MAX_PUNTI ? MAX_PUNTI : r.totale;
  if (r.totale == 0 || r.indice >= tab.totale) {
    tab.completa = true;
    letturaInCorso = false;
  } else {
    tab.freq_hz[r.indice] = r.freq_hz;
    tab.posizione[r.indice] = r.posizione;
    prossimoIndice = r.indice + 1;
    if (prossimoIndice >= tab.totale) {
      tab.completa = true;
      letturaInCorso = false;
    } else {
      chiediProssimo = true;
    }
  }
  tab.versione++;
}

}  // namespace

void inizia(const uint8_t peer[6]) {
  radio.inizia(peer, segreti::PMK, segreti::LMK, cfg::CANALE_WIFI);
}

void invia(proto::Tipo tipo, uint32_t freq_hz, int32_t valore) {
  proto::Comando c = {};
  c.versione = proto::VERSIONE;
  c.tipo = tipo;
  c.freq_hz = freq_hz;
  c.valore = valore;
  portENTER_CRITICAL(&mux);
  if (tipo != proto::Tipo::HEARTBEAT && inAttesa) {  // in coda: parte dopo la risposta al comando in corso
    if (nCoda < MAX_CODA) coda[nCoda++] = c;
    portEXIT_CRITICAL(&mux);
    return;
  }
  c.seq = prossimaSeq();
  if (tipo != proto::Tipo::HEARTBEAT) {
    attesa = c;
    inAttesa = true;
    tentativi = 1;
    istanteInvio = millis();
    fallito = false;
  }
  inviati++;
  portEXIT_CRITICAL(&mux);
  trasmetti(c);
}

void aggiorna() {
  Pacchetto p;
  while (radio.ricevi(p)) {
    if (p.len < 2 || p.dati[0] != proto::VERSIONE) continue;
    proto::Tipo tipo = (proto::Tipo)p.dati[1];
    portENTER_CRITICAL(&mux);
    if (tipo == proto::Tipo::RISPOSTA && p.len == sizeof(proto::Risposta)) {
      proto::Risposta r;
      memcpy(&r, p.dati, sizeof(r));
      riceviStato(r);
    } else if (tipo == proto::Tipo::RISPOSTA_CAL && p.len == sizeof(proto::RispostaCal)) {
      proto::RispostaCal r;
      memcpy(&r, p.dati, sizeof(r));
      riceviPunto(r);
    } else {
      portEXIT_CRITICAL(&mux);
      continue;
    }
    ricevuta = true;
    ricevuti++;
    rssi = p.rssi;
    istanteRisposta = millis();
    portEXIT_CRITICAL(&mux);
  }

  uint32_t ora = millis();

  // ripetizione dell'ultimo comando senza risposta
  bool ripeti = false;
  bool prossimo = false;
  proto::Comando c;
  portENTER_CRITICAL(&mux);
  if (inAttesa && ora - istanteInvio >= cfg::RIPETIZIONE_MS) {
    if (tentativi < cfg::MAX_TENTATIVI) {
      tentativi++;
      istanteInvio = ora;
      ripetuti++;
      c = attesa;
      ripeti = true;
    } else {
      inAttesa = false;
      fallito = true;
      letturaInCorso = false;  // lettura della tabella interrotta
    }
  }
  bool daCoda = false;
  proto::Comando prossimoInCoda;
  if (!inAttesa && nCoda) {
    prossimoInCoda = coda[0];
    for (uint8_t i = 1; i < nCoda; i++) coda[i - 1] = coda[i];
    nCoda--;
    daCoda = true;
  }
  if (chiediProssimo && !inAttesa && !daCoda) {
    chiediProssimo = false;
    prossimo = true;
  }
  uint8_t indice = prossimoIndice;
  bande::Banda banda = tab.banda;
  portEXIT_CRITICAL(&mux);
  if (ripeti) trasmetti(c);
  if (daCoda) invia(prossimoInCoda.tipo, prossimoInCoda.freq_hz, prossimoInCoda.valore);
  if (prossimo) invia(proto::Tipo::CAL_LEGGI, freqDiBanda(banda), indice);

  if (ora - ultimoInvio >= cfg::HEARTBEAT_MS) invia(proto::Tipo::HEARTBEAT);
}

Stato stato() {
  Stato s;
  portENTER_CRITICAL(&mux);
  s.ricevuta = ricevuta;
  s.r = ultima;
  s.etaMs = millis() - istanteRisposta;
  s.comandoFallito = fallito;
  s.rssi = rssi;
  s.ripetuti = ripetuti;
  s.inviati = inviati;
  s.ricevuti = ricevuti;
  portEXIT_CRITICAL(&mux);
  s.collegato = s.ricevuta && s.etaMs < cfg::TIMEOUT_COLLEGAMENTO_MS;
  s.peerImpostato = radio.peerValido();
  return s;
}

void richiediTabella(bande::Banda banda) {
  portENTER_CRITICAL(&mux);
  uint32_t v = tab.versione + 1;
  memset(&tab, 0, sizeof(tab));
  tab.banda = banda;
  tab.versione = v;
  letturaInCorso = true;
  prossimoIndice = 0;
  chiediProssimo = false;
  portEXIT_CRITICAL(&mux);
  invia(proto::Tipo::CAL_LEGGI, freqDiBanda(banda), 0);
}

Tabella tabella() {
  Tabella t;
  portENTER_CRITICAL(&mux);
  t = tab;
  portEXIT_CRITICAL(&mux);
  return t;
}

}  // namespace link_antenna
