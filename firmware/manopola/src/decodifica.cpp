#include "decodifica.h"

#include <Arduino.h>
#include <Decoder.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "generatore.h"
#include "microfono.h"

namespace decodifica {
namespace {

SemaphoreHandle_t mutex = nullptr;
decoder::DecoderCw cw;
decoder::DecoderRtty rtty_;
bool modoRtty = false;
proto::SorgenteAudio sorgente_ = proto::SorgenteAudio::MICROFONO;
proto::ManopolaImposta attuale = {};
bool configurata = false;
volatile float livelloProva = 0;

constexpr size_t MAX_TESTO = 256;
char testo_[MAX_TESTO];
uint32_t nTesto = 0;  // caratteri dall'accensione; testo_ è circolare

const float BAUD[] = {45.45f, 50.0f, 75.0f};
const float STOP[] = {1.0f, 1.5f, 2.0f};

void aggiungi(char c, void*) {  // con il mutex preso
  if (c == '\r') return;
  testo_[nTesto % MAX_TESTO] = c;
  nTesto++;
}

void elabora(const int16_t* x, size_t n) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
  if (modoRtty) rtty_.campioni(x, n);
  else cw.campioni(x, n);
  xSemaphoreGive(mutex);
}

void daMicrofono(const int16_t* x, size_t n) {
  if (sorgente_ == proto::SorgenteAudio::MICROFONO) elabora(x, n);
}

void compitoProva(void*) {  // segnale di prova in tempo reale: 256 campioni ogni 32 ms
  static int16_t x[256];
  float energia = 0;
  TickType_t prossimo = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&prossimo, pdMS_TO_TICKS(32));
    if (sorgente_ != proto::SorgenteAudio::PROVA) {
      livelloProva = 0;
      continue;
    }
    generatore::campioni(x, 256);
    for (int i = 0; i < 256; i++) energia += ((float)x[i] * x[i] - energia) * 0.001f;
    livelloProva = sqrtf(energia) / 32768.0f;
    elabora(x, 256);
  }
}

void applica(const proto::ManopolaImposta& p) {  // con il mutex preso
  modoRtty = p.modo == 1;
  cw.configura((float)cfg::FREQ_AUDIO_HZ, p.cwTono, p.cwFiltro, p.cwWpm, aggiungi, nullptr);
  decoder::DecoderRtty::Parametri r;
  r.baud = BAUD[p.rttyBaud < 3 ? p.rttyBaud : 0];
  r.shift = p.rttyShift;
  r.mark = p.rttyMark;
  r.bitStop = STOP[p.rttyStop < 3 ? p.rttyStop : 1];
  r.reverse = p.rttyOpzioni & 1;
  r.uos = p.rttyOpzioni & 2;
  r.afc = p.rttyOpzioni & 4;
  rtty_.configura((float)cfg::FREQ_AUDIO_HZ, r, aggiungi, nullptr);
  sorgente_ = (proto::SorgenteAudio)(p.sorgente <= 2 ? p.sorgente : 1);
  generatore::Parametri g;
  g.rtty = modoRtty;
  g.fs = (float)cfg::FREQ_AUDIO_HZ;
  g.tono = p.cwTono;
  g.wpm = p.cwWpm ? p.cwWpm : 18;
  g.baud = r.baud;
  g.mark = r.mark;
  g.shift = r.shift;
  g.bitStop = r.bitStop;
  generatore::configura(g);
}

bool uguali(const proto::ManopolaImposta& a, const proto::ManopolaImposta& b) {
  return a.sorgente == b.sorgente && a.modo == b.modo && a.cwTono == b.cwTono && a.cwFiltro == b.cwFiltro &&
         a.cwWpm == b.cwWpm && a.rttyBaud == b.rttyBaud && a.rttyShift == b.rttyShift && a.rttyMark == b.rttyMark &&
         a.rttyStop == b.rttyStop && a.rttyOpzioni == b.rttyOpzioni;
}

}  // namespace

void inizia() {
  mutex = xSemaphoreCreateMutex();
  proto::ManopolaImposta p = {};  // valori predefiniti fino alle impostazioni del pannello
  p.sorgente = (uint8_t)proto::SorgenteAudio::MICROFONO;
  p.cwTono = 700;
  p.cwFiltro = 250;
  p.rttyShift = 170;
  p.rttyMark = 2125;
  p.rttyStop = 1;
  p.rttyOpzioni = 2 | 4;  // UOS e AFC
  xSemaphoreTake(mutex, portMAX_DELAY);
  applica(p);
  attuale = p;
  xSemaphoreGive(mutex);
  microfono::inizia(cfg::FREQ_AUDIO_HZ, daMicrofono);
  xTaskCreatePinnedToCore(compitoProva, "prova", 4096, nullptr, 3, nullptr, 0);
}

void configura(const proto::ManopolaImposta& imp) {
  if (configurata && uguali(imp, attuale)) return;
  xSemaphoreTake(mutex, portMAX_DELAY);
  applica(imp);
  attuale = imp;
  configurata = true;
  xSemaphoreGive(mutex);
}

void riempi(proto::ManopolaStato& s) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  s.seqImposta = attuale.seq;
  s.sorgente = (uint8_t)sorgente_;
  bool ingresso = sorgente_ == proto::SorgenteAudio::PROVA || microfono::attivo();
  s.flag = (ingresso ? proto::AUDIO_INGRESSO : 0) | (modoRtty ? proto::AUDIO_RTTY : 0) |
           (cw.tasto() ? proto::AUDIO_TASTO : 0) | (rtty_.agganciato() ? proto::AUDIO_AGGANCIATO : 0);
  auto a8 = [](float v) { return (uint8_t)(v <= 0 ? 0 : v >= 1 ? 255 : v * 255 + 0.5f); };
  s.audio = a8(livello());
  s.cwLivello = a8(cw.livello());
  s.cwSoglia = a8(cw.soglia());
  float w = cw.wpm();
  s.cwWpm = (uint8_t)(w < 0 ? 0 : w > 99 ? 99 : w + 0.5f);
  s.rttyMark = a8(rtty_.livelloMark());
  s.rttySpace = a8(rtty_.livelloSpace());
  s.rttyAfc = (int16_t)(rtty_.offsetAfc() * 10);
  strncpy(s.cwElementi, cw.elementi(), sizeof(s.cwElementi));
  s.nTesto = nTesto;
  uint32_t l = nTesto < sizeof(s.testo) ? nTesto : sizeof(s.testo);
  s.lTesto = (uint8_t)l;
  for (uint32_t i = 0; i < l; i++) s.testo[i] = testo_[(nTesto - l + i) % MAX_TESTO];
  xSemaphoreGive(mutex);
}

uint32_t testo(char* out, size_t n) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  uint32_t l = nTesto < n - 1 ? nTesto : n - 1;
  if (l > MAX_TESTO) l = MAX_TESTO;
  for (uint32_t i = 0; i < l; i++) {
    char c = testo_[(nTesto - l + i) % MAX_TESTO];
    out[i] = c == '\n' ? ' ' : c;
  }
  out[l] = 0;
  uint32_t v = nTesto;
  xSemaphoreGive(mutex);
  return v;
}

float livello() { return sorgente_ == proto::SorgenteAudio::PROVA ? livelloProva : microfono::livello(); }

bool rtty() { return modoRtty; }

float wpm() { return cw.wpm(); }

bool agganciato() { return rtty_.agganciato(); }

proto::SorgenteAudio sorgente() { return sorgente_; }

}  // namespace decodifica
