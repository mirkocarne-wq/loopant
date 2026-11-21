#include "interfaccia.h"

#include <Arduino.h>
#include <Grafica.h>
#include <associazione.h>
#include <bande.h>
#include <lvgl.h>
#include <stdlib.h>

#include "condiviso.h"
#include "decodifica.h"
#include "encoder.h"

namespace interfaccia {
namespace {

// Colori del tema Notte del pannello
const lv_color_t FONDO = lv_color_hex(0x0B1016);
const lv_color_t TESTO = lv_color_hex(0xE6EDF3);
const lv_color_t TESTO2 = lv_color_hex(0x8B949E);
const lv_color_t ACCENTO = lv_color_hex(0xFFB547);
const lv_color_t SPENTO = lv_color_hex(0x2A2418);
const lv_color_t OK = lv_color_hex(0x3FB950);
const lv_color_t PERICOLO = lv_color_hex(0xF85149);

lv_obj_t* arco;       // livello audio sul bordo
lv_obj_t* modo;       // RITOCCO / FREQUENZA
lv_obj_t* stato;      // collegamento
lv_obj_t* cifre;      // numero grande
lv_obj_t* unita;      // sotto il numero
lv_obj_t* riga2;      // antenna accordata / indicazioni
lv_obj_t* testo;      // testo decodificato
lv_obj_t* decoder;    // CW 18 WPM / RTTY
lv_obj_t* avviso;     // messaggio a tutto schermo (associazione, TUNE)
int32_t ultimoEncoder = 0;
uint32_t avvisoFino = 0;

lv_obj_t* etichetta(lv_obj_t* p, const lv_font_t* f, lv_color_t c, lv_coord_t y) {
  lv_obj_t* l = lv_label_create(p);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, c, 0);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(l, LV_ALIGN_CENTER, 0, y);
  lv_label_set_text(l, "");
  return l;
}

// "_7 074,0" o "14 074,0" per le cifre a 7 segmenti, come la Sintonia del pannello
void cifreKhz(uint32_t hz, char* out, size_t n) {
  uint32_t d = (hz + 50) / 100, k = d / 10;
  char p[8];
  snprintf(p, sizeof(p), "%5lu", (unsigned long)(k % 100000));
  for (char* q = p; *q; q++)
    if (*q == ' ') *q = '_';
  snprintf(out, n, "%c%c %c%c%c,%lu", p[0], p[1], p[2], p[3], p[4], (unsigned long)(d % 10));
}

// "7 074,0" per i testi
void testoKhz(uint32_t hz, char* out, size_t n) {
  uint32_t d = (hz + 50) / 100, k = d / 10;
  if (k >= 1000) snprintf(out, n, "%lu %03lu,%lu", (unsigned long)(k / 1000), (unsigned long)(k % 1000), (unsigned long)(d % 10));
  else snprintf(out, n, "%lu,%lu", (unsigned long)k, (unsigned long)(d % 10));
}

// Passo della frequenza: banda attuale, oltre i bordi si passa all'altra banda
uint32_t passa(uint32_t f, int32_t delta, uint32_t passo) {
  int64_t g = (int64_t)f + (int64_t)delta * passo;
  bande::Banda b = bande::di(f);
  const bande::Info* i = bande::info(b == bande::Banda::NESSUNA ? bande::Banda::B40M : b);
  if (g > (int64_t)i->max_hz) {  // oltre il bordo alto: inizio della banda successiva
    for (size_t k = 0; k < bande::NUMERO; k++)
      if (bande::ELENCO[k].min_hz > i->max_hz) return bande::ELENCO[k].min_hz;
    return bande::ELENCO[0].min_hz;
  }
  if (g < (int64_t)i->min_hz) {  // sotto il bordo basso: fine della banda precedente
    for (size_t k = bande::NUMERO; k-- > 0;)
      if (bande::ELENCO[k].max_hz < i->min_hz) return bande::ELENCO[k].max_hz;
    return bande::ELENCO[bande::NUMERO - 1].max_hz;
  }
  return (uint32_t)g;
}

void mostraAvviso(const char* t, uint32_t ms) {
  lv_label_set_text(avviso, t);
  lv_obj_clear_flag(avviso, LV_OBJ_FLAG_HIDDEN);
  avvisoFino = ms ? millis() + ms : 0;
}

void tocco(lv_event_t* e) {
  condiviso::Comandi c = condiviso::comandi();
  condiviso::Pannello p = condiviso::pannello();
  if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) {
    c.modoFrequenza = !c.modoFrequenza;
    if (c.modoFrequenza) {  // si parte dalla frequenza accordata, o da quella del pannello
      uint32_t f = p.valida ? p.imp.antFreqHz : 0;
      if (bande::di(f) == bande::Banda::NESSUNA && p.valida) f = p.imp.freqProposta;
      if (bande::di(f) == bande::Banda::NESSUNA) f = c.freqModifica;
      if (bande::di(f) == bande::Banda::NESSUNA) f = 7100000;
      c.freqModifica = f;
    }
  } else if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED && c.modoFrequenza) {
    c.nTune++;
    c.freqTune = c.freqModifica;
    c.modoFrequenza = false;  // dopo il TUNE la manopola torna al ritocco fine
    char t[40], f[16];
    testoKhz(c.freqTune, f, sizeof(f));
    snprintf(t, sizeof(t), "TUNE\n%s kHz", f);
    mostraAvviso(t, 1500);
  }
  condiviso::comandi(c);
}

void aggiorna(lv_timer_t*) {
  condiviso::Pannello p = condiviso::pannello();
  condiviso::Comandi c = condiviso::comandi();

  // manopola: ritocco fine o frequenza (più veloce se si gira in fretta)
  int32_t e = encoder::scatti();
  int32_t d = e - ultimoEncoder;
  ultimoEncoder = e;
  if (d) {
    if (c.modoFrequenza) c.freqModifica = passa(c.freqModifica, d, abs(d) >= 3 ? 10000 : 1000);
    else c.scatti += d;
    condiviso::comandi(c);
  }

  // livello audio sul bordo
  float liv = decodifica::livello() * 4;  // il segnale utile è basso rispetto al fondo scala
  lv_arc_set_value(arco, (int16_t)(liv > 1 ? 100 : liv * 100));

  // collegamento
  char t[64];
  if (!p.associato) lv_label_set_text(stato, "non associata");
  else if (!p.collegato) lv_label_set_text(stato, "pannello assente");
  else if (p.valida && !p.imp.antCollegata) lv_label_set_text(stato, "antenna assente");
  else {
    snprintf(t, sizeof(t), "collegata · %d dBm", p.rssi);
    lv_label_set_text(stato, t);
  }
  lv_obj_set_style_text_color(stato, p.collegato && (!p.valida || p.imp.antCollegata) ? OK : PERICOLO, 0);

  // centro
  char n[20];
  if (c.modoFrequenza) {
    lv_label_set_text(modo, "FREQUENZA");
    cifreKhz(c.freqModifica, n, sizeof(n));
    display7::testo(cifre, n);
    lv_label_set_text(unita, "kHz · tocco lungo = TUNE");
  } else {
    lv_label_set_text(modo, "RITOCCO FINE");
    long pos = p.valida ? (long)p.imp.antPosizione : 0;
    if (pos < 0) pos = 0;
    snprintf(n, sizeof(n), "%6ld", pos % 1000000);
    char m[12];
    snprintf(m, sizeof(m), "%.3s %.3s", n, n + 3);
    for (char* q = m; *q; q++)
      if (*q == ' ' && q != m + 3) *q = '_';
    display7::testo(cifre, p.valida ? m : "___ ___");
    snprintf(t, sizeof(t), "passi · %u per scatto", p.valida ? p.imp.passiScatto : 0);
    lv_label_set_text(unita, t);
  }
  if (p.valida && p.imp.antFreqHz) {
    testoKhz(p.imp.antFreqHz, n, sizeof(n));
    snprintf(t, sizeof(t), "accordata su %s kHz", n);
    lv_label_set_text(riga2, t);
  } else {
    lv_label_set_text(riga2, p.valida && p.imp.antCollegata ? "antenna non accordata" : "");
  }

  // decoder
  static char buf[64];
  decodifica::testo(buf, 21);
  lv_label_set_text(testo, buf);
  if (decodifica::rtty()) lv_label_set_text(decoder, decodifica::agganciato() ? "RTTY · agganciato" : "RTTY · in ascolto");
  else {
    snprintf(t, sizeof(t), "CW · %d WPM", (int)(decodifica::wpm() + 0.5f));
    lv_label_set_text(decoder, t);
  }

  // avvisi: associazione e TUNE
  using associazione::Fase;
  Fase f = (Fase)p.associazione;
  if (f == Fase::RICERCA) mostraAvviso("Cerco il pannello…", 0);
  else if (f == Fase::ATTESA) mostraAvviso("Conferma\nsul pannello", 0);
  else if (f == Fase::RIUSCITA) mostraAvviso("Associata", 2000);
  else if (f == Fase::RIFIUTATA) mostraAvviso("Rifiutata", 2000);
  else if (avvisoFino ? (int32_t)(millis() - avvisoFino) > 0 : f == Fase::INATTIVA)
    lv_obj_add_flag(avviso, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace

void crea() {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, FONDO, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  arco = lv_arc_create(scr);
  lv_obj_set_size(arco, 350, 350);
  lv_obj_center(arco);
  lv_arc_set_bg_angles(arco, 135, 45);
  lv_arc_set_range(arco, 0, 100);
  lv_arc_set_value(arco, 0);
  lv_obj_remove_style(arco, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(arco, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(arco, 8, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arco, 8, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arco, lv_color_hex(0x1C2430), LV_PART_MAIN);
  lv_obj_set_style_arc_color(arco, OK, LV_PART_INDICATOR);

  modo = etichetta(scr, &font_18, ACCENTO, -118);
  stato = etichetta(scr, &font_14, TESTO2, -92);
  cifre = display7::crea(scr, 52, ACCENTO, SPENTO);
  lv_obj_align(cifre, LV_ALIGN_CENTER, 0, -30);
  unita = etichetta(scr, &font_14, TESTO2, 14);
  riga2 = etichetta(scr, &font_16, TESTO, 42);
  testo = etichetta(scr, &font_22, TESTO, 82);
  lv_obj_set_width(testo, 250);
  lv_label_set_long_mode(testo, LV_LABEL_LONG_CLIP);
  decoder = etichetta(scr, &font_14, TESTO2, 116);

  avviso = lv_label_create(scr);
  lv_obj_set_size(avviso, 360, 360);
  lv_obj_center(avviso);
  lv_obj_set_style_bg_color(avviso, FONDO, 0);
  lv_obj_set_style_bg_opa(avviso, LV_OPA_90, 0);
  lv_obj_set_style_text_font(avviso, &font_28, 0);
  lv_obj_set_style_text_color(avviso, ACCENTO, 0);
  lv_obj_set_style_text_align(avviso, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_pad_top(avviso, 140, 0);
  lv_obj_add_flag(avviso, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(avviso, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_add_event_cb(scr, tocco, LV_EVENT_SHORT_CLICKED, nullptr);
  lv_obj_add_event_cb(scr, tocco, LV_EVENT_LONG_PRESSED, nullptr);
  lv_timer_create(aggiorna, 50, nullptr);
  ultimoEncoder = encoder::scatti();
}

}  // namespace interfaccia
