#include "link_manopola.h"

#include <Arduino.h>
#include <Preferences.h>
#include <bande.h>
#include <collegamento.h>
#include <segreti_scelti.h>
#include <string.h>

#include "config.h"
#include "decodifica.h"
#include "impostazioni.h"
#include "link_antenna.h"

namespace link_manopola {
namespace {

constexpr uint32_t IMPOSTA_MS = 1000;     // impostazioni e dati dell'antenna (anche heartbeat)
constexpr uint32_t SILENZIO_MS = 3000;

Collegamento radio;
Preferences nvs;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
Scelte scelte_ = {(uint8_t)proto::SorgenteAudio::MICROFONO, 20};
uint8_t macPeer[6] = {0};
uint16_t seq = 1;
volatile bool inviaSubito = false;
uint32_t ultimoInvio = 0, ultimaRicezione = 0;
int8_t rssi = 0;
proto::ManopolaStato ultimo = {};
bool primo = true;  // primo stato dopo l'avvio o l'associazione: allinea i contatori senza agire
int32_t scattiFatti = 0;
uint16_t nTuneVisto = 0, nZeroVisto = 0;
uint32_t testoVisto = 0;

void inviaImpostazioni() {
  const Impostazioni& imp = impostazioni::get();
  link_antenna::Stato a = link_antenna::stato();
  proto::ManopolaImposta m = {};
  m.versione = proto::VERSIONE;
  m.tipo = proto::Tipo::MANOPOLA_IMPOSTA;
  m.seq = seq;
  m.sorgente = scelte_.sorgente;
  m.modo = imp.decModo;
  m.cwTono = imp.cwTono;
  m.cwFiltro = imp.cwFiltro;
  m.cwWpm = imp.cwWpm;
  m.rttyBaud = imp.rttyBaud;
  m.rttyShift = imp.rttyShift;
  m.rttyMark = imp.rttyMark;
  m.rttyStop = imp.rttyStop;
  m.rttyOpzioni = imp.rttyOpzioni;
  m.antCollegata = a.collegato;
  m.antStato = (uint8_t)a.r.stato;
  m.antFlag = a.r.flag;
  m.antPosizione = a.r.posizione;
  m.antFreqHz = a.r.freq_hz;
  m.freqProposta = imp.ultima40_hz;
  m.passiScatto = scelte_.passiScatto;
  radio.invia(&m, sizeof(m));
  ultimoInvio = millis();
}

// Caratteri nuovi del decoder della manopola (anche se si sono persi dei pacchetti)
void prendiTesto(const proto::ManopolaStato& s) {
  if (s.nTesto == testoVisto) return;
  uint32_t nuovi = s.nTesto - testoVisto;
  if (s.nTesto < testoVisto) nuovi = s.nTesto;  // la manopola si è riavviata
  char buf[52];
  size_t k = 0;
  if (nuovi > s.lTesto) {  // persi: si segna il salto
    buf[k++] = ' ';
    nuovi = s.lTesto;
  }
  for (uint32_t i = s.lTesto - nuovi; i < s.lTesto && k < sizeof(buf); i++) buf[k++] = s.testo[i];
  decodifica::aggiungiRemoto(buf, k);
  testoVisto = s.nTesto;
}

// Scatti della manopola -> JOG, solo ad antenna ferma e azzerata (il resto si accumula)
void eseguiComandi(const proto::ManopolaStato& s) {
  link_antenna::Stato a = link_antenna::stato();
  bool pronta = a.collegato && a.r.stato == proto::Stato::FERMO && (a.r.flag & proto::FLAG_AZZERATA);
  int32_t da = s.scatti - scattiFatti;
  if (da && pronta) {
    link_antenna::invia(proto::Tipo::JOG, 0, da * (int32_t)scelte_.passiScatto);
    scattiFatti = s.scatti;
  } else if (da && a.collegato && a.r.stato != proto::Stato::IN_MOVIMENTO && !(a.r.flag & proto::FLAG_AZZERATA)) {
    scattiFatti = s.scatti;  // antenna non azzerata: il ritocco non ha senso, si scarta
  }
  if (s.nTune != nTuneVisto) {
    nTuneVisto = s.nTune;
    if (bande::di(s.freqTune) != bande::Banda::NESSUNA) link_antenna::invia(proto::Tipo::GOTO, s.freqTune);
  }
  if (s.nZero != nZeroVisto) {
    nZeroVisto = s.nZero;
    link_antenna::invia(proto::Tipo::ZERO);
  }
}

}  // namespace

void inizia() {
  nvs.begin("manopola", false);
  nvs.getBytes("peer", macPeer, 6);
  scelte_.sorgente = nvs.getUChar("sorgente", (uint8_t)proto::SorgenteAudio::MICROFONO);
  scelte_.passiScatto = nvs.getUShort("passi", 20);
  if (scelte_.sorgente > 2) scelte_.sorgente = (uint8_t)proto::SorgenteAudio::MICROFONO;
  if (!scelte_.passiScatto) scelte_.passiScatto = 20;
  radio.inizia(macPeer, segreti::PMK, segreti::LMK, cfg::CANALE_WIFI);
}

void aggiorna() {
  Pacchetto p;
  while (radio.ricevi(p)) {
    if (p.len != sizeof(proto::ManopolaStato)) continue;
    proto::ManopolaStato s;
    memcpy(&s, p.dati, sizeof(s));
    if (s.versione != proto::VERSIONE || s.tipo != proto::Tipo::MANOPOLA_STATO) continue;
    if (primo) {  // contatori allineati: niente comandi per quello che è successo prima
      primo = false;
      scattiFatti = s.scatti;
      nTuneVisto = s.nTune;
      nZeroVisto = s.nZero;
      testoVisto = s.nTesto;
    }
    prendiTesto(s);
    eseguiComandi(s);
    portENTER_CRITICAL(&mux);
    ultimo = s;
    rssi = p.rssi;
    ultimaRicezione = millis();
    portEXIT_CRITICAL(&mux);
  }
  if (radio.peerValido() && (inviaSubito || millis() - ultimoInvio >= IMPOSTA_MS)) {
    inviaSubito = false;
    inviaImpostazioni();
  }
}

Stato stato() {
  Stato s;
  portENTER_CRITICAL(&mux);
  s.s = ultimo;
  s.rssi = rssi;
  s.collegata = ultimaRicezione && millis() - ultimaRicezione < SILENZIO_MS;
  portEXIT_CRITICAL(&mux);
  s.associata = radio.peerValido();
  return s;
}

Scelte& scelte() { return scelte_; }

void salvaScelte() {
  nvs.putUChar("sorgente", scelte_.sorgente);
  nvs.putUShort("passi", scelte_.passiScatto);
  impostazioniCambiate();
}

void impostazioniCambiate() {
  seq++;
  inviaSubito = true;
}

bool stessoMac(const uint8_t mac[6]) { return radio.peerValido() && Collegamento::stessoMac(mac, macPeer); }

void associa(const uint8_t mac[6]) {
  memcpy(macPeer, mac, 6);
  nvs.putBytes("peer", macPeer, 6);
  radio.cambiaPeer(macPeer, segreti::LMK);
  primo = true;
  inviaSubito = true;
}

void dimentica() {
  memset(macPeer, 0, 6);
  nvs.putBytes("peer", macPeer, 6);
  radio.cambiaPeer(macPeer, segreti::LMK);
}

}  // namespace link_manopola
