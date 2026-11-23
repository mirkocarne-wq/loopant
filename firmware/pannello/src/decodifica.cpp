#include "decodifica.h"

#include <Arduino.h>
#include <Preferences.h>
#include <Decoder.h>
#include <string.h>

#include "audio.h"
#include "config.h"
#include "impostazioni.h"
#include "link_manopola.h"

namespace decodifica {
namespace {

SemaphoreHandle_t mutex = nullptr;
decoder::DecoderCw cw;
decoder::DecoderRtty rtty;
bool modoRtty = false;
volatile bool attivo_ = true;

constexpr size_t MAX_TESTO = 1200;
char testo_[MAX_TESTO + 1] = "";
size_t lunghezza = 0;
uint32_t versione = 0;

void aggiungi(char c, void*) {  // chiamata con il mutex preso
  if (c == '\n' && lunghezza && testo_[lunghezza - 1] == '\n') return;
  if (lunghezza >= MAX_TESTO) {  // toglie la prima metà
    memmove(testo_, testo_ + MAX_TESTO / 2, lunghezza - MAX_TESTO / 2);
    lunghezza -= MAX_TESTO / 2;
  }
  testo_[lunghezza++] = c;
  testo_[lunghezza] = 0;
  versione++;
}

void elabora(const int16_t* x, size_t n) {
  if (!attivo_) return;                           // decoder spento
  if (link_manopola::stato().associata) return;  // decodifica la manopola
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
  if (modoRtty) rtty.campioni(x, n);
  else cw.campioni(x, n);
  xSemaphoreGive(mutex);
}

const float BAUD[] = {45.45f, 50.0f, 75.0f};
const float STOP[] = {1.0f, 1.5f, 2.0f};

}  // namespace

void inizia() {
  if (!mutex) mutex = xSemaphoreCreateMutex();
  Preferences p;
  p.begin("decoder", true);
  attivo_ = p.getBool("attivo", true);
  p.end();
  configura();
  audio::inizia(cfg::PIN_AUDIO, cfg::FREQ_AUDIO_HZ, elabora);
}

void configura() {
  const Impostazioni& imp = impostazioni::get();
  xSemaphoreTake(mutex, portMAX_DELAY);
  modoRtty = imp.decModo == 1;
  cw.configura((float)cfg::FREQ_AUDIO_HZ, imp.cwTono, imp.cwFiltro, imp.cwWpm, aggiungi, nullptr);
  decoder::DecoderRtty::Parametri p;
  p.baud = BAUD[imp.rttyBaud < 3 ? imp.rttyBaud : 0];
  p.shift = imp.rttyShift;
  p.mark = imp.rttyMark;
  p.bitStop = STOP[imp.rttyStop < 3 ? imp.rttyStop : 1];
  p.reverse = imp.rttyOpzioni & impostazioni::RTTY_REVERSE;
  p.uos = imp.rttyOpzioni & impostazioni::RTTY_UOS;
  p.afc = imp.rttyOpzioni & impostazioni::RTTY_AFC;
  rtty.configura((float)cfg::FREQ_AUDIO_HZ, p, aggiungi, nullptr);
  xSemaphoreGive(mutex);
  link_manopola::impostazioniCambiate();
}

Stato stato() {
  Stato s = {};
  s.attivo = attivo_;
  link_manopola::Stato m = link_manopola::stato();
  if (m.associata) {  // stato del decoder della manopola (valori 0-255 -> 0-1)
    const proto::ManopolaStato& r = m.s;
    s.remoto = true;
    s.remotoCollegato = m.collegata;
    s.sorgente = r.sorgente;
    s.ingresso = m.collegata && (r.flag & proto::AUDIO_INGRESSO);
    s.audio = r.audio / 255.0f;
    s.rtty = r.flag & proto::AUDIO_RTTY;
    s.cwLivello = r.cwLivello / 255.0f;
    s.cwSoglia = r.cwSoglia / 255.0f;
    s.cwWpm = r.cwWpm;
    s.cwTasto = r.flag & proto::AUDIO_TASTO;
    strncpy(s.cwElementi, r.cwElementi, sizeof(s.cwElementi) - 1);
    s.rttyMark = r.rttyMark / 255.0f;
    s.rttySpace = r.rttySpace / 255.0f;
    s.rttyAfc = r.rttyAfc / 10.0f;
    s.rttyAgganciato = r.flag & proto::AUDIO_AGGANCIATO;
    return s;
  }
  s.ingresso = audio::attivo();
  s.audio = audio::livello();
  xSemaphoreTake(mutex, portMAX_DELAY);
  s.rtty = modoRtty;
  s.cwLivello = cw.livello();
  s.cwSoglia = cw.soglia();
  s.cwWpm = cw.wpm();
  s.cwTasto = cw.tasto();
  strncpy(s.cwElementi, cw.elementi(), sizeof(s.cwElementi) - 1);
  s.rttyMark = rtty.livelloMark();
  s.rttySpace = rtty.livelloSpace();
  s.rttyAfc = rtty.offsetAfc();
  s.rttyAgganciato = rtty.agganciato();
  xSemaphoreGive(mutex);
  return s;
}

uint32_t testo(char* out, size_t n) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  size_t da = lunghezza >= n ? lunghezza - (n - 1) : 0;
  strncpy(out, testo_ + da, n - 1);
  out[n - 1] = 0;
  uint32_t v = versione;
  xSemaphoreGive(mutex);
  return v;
}

bool attivo() { return attivo_; }

void attiva(bool si) {
  if (si == attivo_) return;
  attivo_ = si;
  Preferences p;
  p.begin("decoder", false);
  p.putBool("attivo", si);
  p.end();
  link_manopola::impostazioniCambiate();  // la manopola lo sa subito
}

void aggiungiRemoto(const char* t, size_t n) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (size_t i = 0; i < n; i++) aggiungi(t[i], nullptr);
  xSemaphoreGive(mutex);
}

void pulisci() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  lunghezza = 0;
  testo_[0] = 0;
  versione++;
  xSemaphoreGive(mutex);
}

}  // namespace decodifica
