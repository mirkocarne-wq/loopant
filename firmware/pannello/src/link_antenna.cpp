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
uint32_t persi = 0;
uint32_t inviati = 0;

uint16_t prossimaSeq() {
  if (++seq == 0) seq = 1;  // 0 è riservato alle risposte spontanee
  return seq;
}

void trasmetti(const proto::Comando& c) {
  radio.invia(&c, sizeof(c));
  ultimoInvio = millis();
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
    if (p.len != sizeof(proto::Risposta)) continue;
    proto::Risposta r;
    memcpy(&r, p.dati, sizeof(r));
    if (r.versione != proto::VERSIONE || r.tipo != proto::Tipo::RISPOSTA) continue;
    portENTER_CRITICAL(&mux);
    ultima = r;
    ricevuta = true;
    istanteRisposta = millis();
    if (inAttesa && r.seq_comando == attesa.seq) inAttesa = false;
    portEXIT_CRITICAL(&mux);
  }

  uint32_t ora = millis();

  // ripetizione dell'ultimo comando senza risposta
  bool ripeti = false;
  proto::Comando c;
  portENTER_CRITICAL(&mux);
  if (inAttesa && ora - istanteInvio >= cfg::RIPETIZIONE_MS) {
    if (tentativi < cfg::MAX_TENTATIVI) {
      tentativi++;
      istanteInvio = ora;
      persi++;
      c = attesa;
      ripeti = true;
    } else {
      inAttesa = false;
      fallito = true;
    }
  }
  portEXIT_CRITICAL(&mux);
  if (ripeti) trasmetti(c);

  if (ora - ultimoInvio >= cfg::HEARTBEAT_MS) invia(proto::Tipo::HEARTBEAT);
}

Stato stato() {
  Stato s;
  portENTER_CRITICAL(&mux);
  s.ricevuta = ricevuta;
  s.r = ultima;
  s.etaMs = millis() - istanteRisposta;
  s.comandoFallito = fallito;
  s.persi = persi;
  s.inviati = inviati;
  portEXIT_CRITICAL(&mux);
  s.collegato = s.ricevuta && s.etaMs < cfg::TIMEOUT_COLLEGAMENTO_MS;
  s.peerImpostato = radio.peerValido();
  return s;
}

}  // namespace link_antenna
