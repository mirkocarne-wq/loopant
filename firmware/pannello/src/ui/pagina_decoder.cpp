// Schermata Decoder CW / RTTY (requisiti D1-D3, docs/software/08-decoder.md).
#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "../decodifica.h"
#include <Grafica.h>
#include "../impostazioni.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"

namespace pag_decoder {
namespace {

lv_obj_t* btnModo[2];
lv_obj_t* chipStato;
lv_obj_t* testoAudio;
lv_obj_t* areaTesto;
lv_obj_t* testo;
lv_obj_t* titoloTesto;
uint32_t versioneTesto = 0xFFFFFFFF;

lv_obj_t* pannelloCw;
lv_obj_t* pannelloRtty;

// CW
lv_obj_t* dWpm;
lv_obj_t* testoWpm;
lv_obj_t* dTono;
lv_obj_t* barraCw;
lv_obj_t* sogliaCw;
lv_obj_t* elementiCw;

// RTTY
lv_obj_t* dBaud;
lv_obj_t* dShift;
lv_obj_t* testoMark;
lv_obj_t* testoSpace;
lv_obj_t* barraMark;
lv_obj_t* barraSpace;
lv_obj_t* testoAfc;

// Pulsanti a scelta (valore come uint16 per coprire filtro e shift)
struct Scelta {
  lv_obj_t* b[3];
  uint8_t n;
  uint16_t valori[3];
  void* campo;  // uint8_t* o uint16_t*
  bool largo;   // campo a 16 bit
};
Scelta scelte[4];
uint8_t numScelte = 0;
lv_obj_t* opzioni[3];

void aggiornaParametri();

uint16_t leggi(const Scelta& s) { return s.largo ? *(uint16_t*)s.campo : *(uint8_t*)s.campo; }

void evidenzia(Scelta& s) {
  for (uint8_t i = 0; i < s.n; i++)
    ui::stilePulsante(s.b[i], leggi(s) == s.valori[i] ? ui::Stile::ATTIVO : ui::Stile::NORMALE);
}

void premiScelta(lv_event_t* e) {
  uintptr_t c = (uintptr_t)lv_event_get_user_data(e);
  Scelta& s = scelte[c >> 4];
  uint16_t v = s.valori[c & 0x0F];
  if (s.largo) *(uint16_t*)s.campo = v;
  else *(uint8_t*)s.campo = (uint8_t)v;
  impostazioni::salva();
  decodifica::configura();
  evidenzia(s);
  aggiornaParametri();
}

void scelta(lv_obj_t* parent, const char* const* nomi, const uint16_t* valori, uint8_t n, void* campo, bool largo) {
  uint8_t id = numScelte++;
  Scelta& s = scelte[id];
  s.n = n;
  s.campo = campo;
  s.largo = largo;
  lv_obj_t* r = ui::contenitore(parent, LV_FLEX_FLOW_ROW, 4);
  lv_obj_set_width(r, LV_PCT(100));
  for (uint8_t i = 0; i < n; i++) {
    s.valori[i] = valori[i];
    lv_obj_t* b = ui::pulsante(r, nomi[i], &font_14, ui::Stile::NORMALE);
    lv_obj_set_height(b, 44);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_add_event_cb(b, premiScelta, LV_EVENT_CLICKED, (void*)(uintptr_t)((id << 4) | i));
    s.b[i] = b;
  }
  evidenzia(s);
}

lv_obj_t* tessera(lv_obj_t* parent, const char* titolo, lv_coord_t larghezza) {
  lv_obj_t* t = ui::scheda(parent);
  lv_obj_set_style_pad_all(t, 10, 0);
  lv_obj_set_style_pad_row(t, 6, 0);
  lv_obj_set_height(t, LV_PCT(100));
  if (larghezza) lv_obj_set_width(t, larghezza);
  else lv_obj_set_flex_grow(t, 1);
  ui::titoletto(t, titolo);
  return t;
}

lv_obj_t* barra(lv_obj_t* parent, lv_color_t colore) {
  const Tema& t = tema::corrente();
  lv_obj_t* b = lv_bar_create(parent);
  lv_obj_set_size(b, LV_PCT(100), 14);
  lv_bar_set_range(b, 0, 100);
  lv_obj_set_style_bg_color(b, t.surface2, 0);
  lv_obj_set_style_bg_color(b, colore, LV_PART_INDICATOR);
  lv_obj_set_style_radius(b, 4, 0);
  lv_obj_set_style_radius(b, 4, LV_PART_INDICATOR);
  return b;
}

void premiModo(lv_event_t* e) {
  impostazioni::get().decModo = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  impostazioni::salva();
  decodifica::configura();
  aggiornaParametri();
}

void premiPulisci(lv_event_t*) { decodifica::pulisci(); }

void premiTono(lv_event_t* e) {
  Impostazioni& imp = impostazioni::get();
  int v = imp.cwTono + (int)(intptr_t)lv_event_get_user_data(e);
  imp.cwTono = (uint16_t)(v < 300 ? 300 : v > 1500 ? 1500 : v);
  impostazioni::salva();
  decodifica::configura();
  aggiornaParametri();
}

void premiAuto(lv_event_t*) {
  Impostazioni& imp = impostazioni::get();
  decodifica::Stato s = decodifica::stato();
  imp.cwWpm = imp.cwWpm ? 0 : (uint8_t)(s.cwWpm + 0.5f);  // fissa la velocità stimata oppure torna in automatico
  impostazioni::salva();
  decodifica::configura();
  aggiornaParametri();
}

void premiMark(lv_event_t* e) {
  Impostazioni& imp = impostazioni::get();
  int v = imp.rttyMark + (int)(intptr_t)lv_event_get_user_data(e);
  imp.rttyMark = (uint16_t)(v < 300 ? 300 : v > 3000 ? 3000 : v);
  impostazioni::salva();
  decodifica::configura();
  aggiornaParametri();
}

void premiOpzione(lv_event_t* e) {
  Impostazioni& imp = impostazioni::get();
  imp.rttyOpzioni ^= (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  impostazioni::salva();
  decodifica::configura();
  aggiornaParametri();
}

void aggiornaParametri() {
  const Impostazioni& imp = impostazioni::get();
  bool r = imp.decModo == 1;
  for (int i = 0; i < 2; i++) ui::stilePulsante(btnModo[i], (i == 1) == r ? ui::Stile::PRIMARIO : ui::Stile::NORMALE);
  if (r) {
    lv_obj_add_flag(pannelloCw, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(pannelloRtty, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(pannelloCw, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(pannelloRtty, LV_OBJ_FLAG_HIDDEN);
  }
  lv_label_set_text(titoloTesto, r ? "TESTO DECODIFICATO · RTTY" : "TESTO DECODIFICATO · CW");
  char buf[16];
  snprintf(buf, sizeof(buf), "%u", imp.cwTono);
  display7::testo(dTono, buf);
  const char* baud[] = {"45,45", "50,00", "75,00"};
  display7::testo(dBaud, baud[imp.rttyBaud < 3 ? imp.rttyBaud : 0]);
  snprintf(buf, sizeof(buf), "%u", imp.rttyShift);
  display7::testo(dShift, buf);
  bool rev = imp.rttyOpzioni & impostazioni::RTTY_REVERSE;
  unsigned mark = rev ? imp.rttyMark + imp.rttyShift : imp.rttyMark;
  unsigned space = rev ? imp.rttyMark : imp.rttyMark + imp.rttyShift;
  lv_label_set_text_fmt(testoMark, "Mark %u Hz", mark);
  lv_label_set_text_fmt(testoSpace, "Space %u Hz", space);
  const uint8_t bit[] = {impostazioni::RTTY_REVERSE, impostazioni::RTTY_UOS, impostazioni::RTTY_AFC};
  for (int i = 0; i < 3; i++)
    ui::stilePulsante(opzioni[i], (imp.rttyOpzioni & bit[i]) ? ui::Stile::ATTIVO : ui::Stile::NORMALE);
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  Impostazioni& imp = impostazioni::get();
  numScelte = 0;
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 10, 0);
  lv_obj_set_style_pad_top(c, 12, 0);
  lv_obj_set_style_pad_bottom(c, 12, 0);

  // Barra del decoder
  lv_obj_t* su = ui::contenitore(c, LV_FLEX_FLOW_ROW, 12);
  lv_obj_set_width(su, LV_PCT(100));
  lv_obj_set_flex_align(su, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  const char* modi[] = {"CW", "RTTY"};
  for (int i = 0; i < 2; i++) {
    btnModo[i] = ui::pulsante(su, modi[i], &font_18, ui::Stile::NORMALE);
    lv_obj_set_size(btnModo[i], 120, 44);
    lv_obj_add_event_cb(btnModo[i], premiModo, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  chipStato = ui::chip(su, &font_14);
  testoAudio = ui::etichetta(su, "", &font_14, t.text2);
  lv_obj_set_flex_grow(testoAudio, 1);
  lv_obj_t* pul = ui::pulsante(su, "Pulisci", &font_16, ui::Stile::NORMALE);
  lv_obj_set_size(pul, 110, 44);
  lv_obj_add_event_cb(pul, premiPulisci, LV_EVENT_CLICKED, nullptr);

  // Testo
  areaTesto = ui::scheda(c);
  lv_obj_set_width(areaTesto, LV_PCT(100));
  lv_obj_set_flex_grow(areaTesto, 1);
  lv_obj_set_style_bg_color(areaTesto, tema::indiceCorrente() == 1 ? t.surface : t.bg, 0);
  lv_obj_set_style_pad_row(areaTesto, 4, 0);
  lv_obj_add_flag(areaTesto, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(areaTesto, LV_DIR_VER);
  titoloTesto = ui::titoletto(areaTesto, "");
  testo = ui::etichetta(areaTesto, "", &font_24, t.text);
  lv_label_set_long_mode(testo, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(testo, LV_PCT(100));
  lv_obj_set_style_text_letter_space(testo, 1, 0);

  // Pannello CW
  pannelloCw = ui::contenitore(c, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_size(pannelloCw, LV_PCT(100), 128);
  lv_obj_t* v = tessera(pannelloCw, "VELOCITÀ", 210);
  lv_obj_t* rv = ui::contenitore(v, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_flex_align(rv, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  dWpm = display7::crea(rv, 44, t.accent, t.segOffAccent);
  display7::testo(dWpm, "__");
  testoWpm = ui::etichetta(rv, "WPM", &font_14, t.text2);
  lv_obj_t* au = ui::pulsante(v, "Auto / fissa", &font_14, ui::Stile::NORMALE);
  lv_obj_set_size(au, LV_PCT(100), 34);
  lv_obj_add_event_cb(au, premiAuto, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* to = tessera(pannelloCw, "TONO · Hz", 220);
  dTono = display7::crea(to, 44, t.accent, t.segOffAccent);
  lv_obj_t* rt = ui::contenitore(to, LV_FLEX_FLOW_ROW, 6);
  lv_obj_set_width(rt, LV_PCT(100));
  const int passiTono[] = {-10, 10};
  const char* nomiTono[] = {"−10", "+10"};
  for (int i = 0; i < 2; i++) {
    lv_obj_t* b = ui::pulsante(rt, nomiTono[i], &font_14, ui::Stile::NORMALE);
    lv_obj_set_height(b, 34);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_add_event_cb(b, premiTono, LV_EVENT_CLICKED, (void*)(intptr_t)passiTono[i]);
  }

  lv_obj_t* li = tessera(pannelloCw, "LIVELLO DEL TONO", 0);
  lv_obj_t* zona = lv_obj_create(li);
  lv_obj_remove_style_all(zona);
  lv_obj_set_size(zona, LV_PCT(100), 22);
  barraCw = barra(zona, t.ok);
  lv_obj_set_height(barraCw, 22);
  sogliaCw = lv_obj_create(zona);
  lv_obj_remove_style_all(sogliaCw);
  lv_obj_set_size(sogliaCw, 3, 26);
  lv_obj_set_style_bg_color(sogliaCw, t.accent, 0);
  lv_obj_set_style_bg_opa(sogliaCw, LV_OPA_COVER, 0);
  elementiCw = ui::etichetta(li, "", &font_30, t.accent);

  lv_obj_t* fi = tessera(pannelloCw, "FILTRO", 230);
  static const char* const FILTRI[] = {"100 Hz", "250 Hz", "500 Hz"};
  static const uint16_t V_FILTRI[] = {100, 250, 500};
  scelta(fi, FILTRI, V_FILTRI, 3, &imp.cwFiltro, true);

  // Pannello RTTY
  pannelloRtty = ui::contenitore(c, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_size(pannelloRtty, LV_PCT(100), 170);
  lv_obj_t* ba = tessera(pannelloRtty, "VELOCITÀ · BAUD", 230);
  dBaud = display7::crea(ba, 40, t.accent, t.segOffAccent);
  static const char* const BAUD[] = {"45,45", "50", "75"};
  static const uint16_t V_BAUD[] = {0, 1, 2};
  scelta(ba, BAUD, V_BAUD, 3, &imp.rttyBaud, false);

  lv_obj_t* sh = tessera(pannelloRtty, "SHIFT · Hz", 210);
  dShift = display7::crea(sh, 40, t.accent, t.segOffAccent);
  static const char* const SHIFT[] = {"170", "425", "850"};
  static const uint16_t V_SHIFT[] = {170, 425, 850};
  scelta(sh, SHIFT, V_SHIFT, 3, &imp.rttyShift, true);

  lv_obj_t* to2 = tessera(pannelloRtty, "TONI MARK / SPACE", 0);
  testoMark = ui::etichetta(to2, "", &font_14, t.text);
  barraMark = barra(to2, t.accent);
  testoSpace = ui::etichetta(to2, "", &font_14, t.text);
  barraSpace = barra(to2, t.ok);
  lv_obj_t* rm = ui::contenitore(to2, LV_FLEX_FLOW_ROW, 4);
  lv_obj_set_width(rm, LV_PCT(100));
  const int passiMark[] = {-10, -1, 1, 10};
  const char* nomiMark[] = {"−10", "−1", "+1", "+10"};
  for (int i = 0; i < 4; i++) {
    lv_obj_t* b = ui::pulsante(rm, nomiMark[i], &font_14, ui::Stile::NORMALE);
    lv_obj_set_height(b, 36);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_add_event_cb(b, premiMark, LV_EVENT_CLICKED, (void*)(intptr_t)passiMark[i]);
  }

  lv_obj_t* op = tessera(pannelloRtty, "BIT DI STOP E OPZIONI", 250);
  static const char* const STOP[] = {"1", "1,5", "2"};
  static const uint16_t V_STOP[] = {0, 1, 2};
  scelta(op, STOP, V_STOP, 3, &imp.rttyStop, false);
  lv_obj_t* ro = ui::contenitore(op, LV_FLEX_FLOW_ROW, 4);
  lv_obj_set_width(ro, LV_PCT(100));
  const char* nomiOpz[] = {"Reverse", "UOS", "AFC"};
  const uint8_t bitOpz[] = {impostazioni::RTTY_REVERSE, impostazioni::RTTY_UOS, impostazioni::RTTY_AFC};
  for (int i = 0; i < 3; i++) {
    opzioni[i] = ui::pulsante(ro, nomiOpz[i], &font_14, ui::Stile::NORMALE);
    lv_obj_set_height(opzioni[i], 44);
    lv_obj_set_flex_grow(opzioni[i], 1);
    lv_obj_add_event_cb(opzioni[i], premiOpzione, LV_EVENT_CLICKED, (void*)(uintptr_t)bitOpz[i]);
  }
  testoAfc = ui::etichetta(op, "", &font_12, t.text2);

  versioneTesto = 0xFFFFFFFF;
  aggiornaParametri();
}

void aggiorna(const link_antenna::Stato&) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  decodifica::Stato s = decodifica::stato();

  static const char* const SORGENTE[] = {"segnale di prova", "microfono", "linea"};
  const char* sorgente = SORGENTE[s.sorgente <= 2 ? s.sorgente : 1];
  if (s.remoto && !s.remotoCollegato) {
    lv_label_set_text(chipStato, "Manopola non collegata");
    ui::coloriChip(chipStato, t.dangerBg, t.dangerText);
    lv_label_set_text(testoAudio, "Il decoder è sulla manopola remota");
  } else if (!s.ingresso) {
    lv_label_set_text(chipStato, "Ingresso audio non attivo");
    ui::coloriChip(chipStato, t.dangerBg, t.dangerText);
    if (s.remoto) lv_label_set_text_fmt(testoAudio, "Manopola: %s non attivo", sorgente);
    else lv_label_set_text_fmt(testoAudio, "GPIO %d (connettore J8)", cfg::PIN_AUDIO);
  } else {
    bool segnale = s.rtty ? s.rttyAgganciato : s.cwLivello > 0.05f;
    lv_label_set_text(chipStato, s.rtty ? (s.rttyAgganciato ? "• Agganciato" : "• In ascolto") : "• In ascolto");
    ui::coloriChip(chipStato, segnale ? t.okBg : t.surface2, segnale ? t.okText : t.text2);
    if (s.remoto) lv_label_set_text_fmt(testoAudio, "Manopola · %s · audio %d %%", sorgente, (int)(s.audio * 100 + 0.5f));
    else lv_label_set_text_fmt(testoAudio, "Audio %d %%", (int)(s.audio * 100 + 0.5f));
  }

  static char buf[1300];
  uint32_t v = decodifica::testo(buf, sizeof(buf));
  if (v != versioneTesto) {
    versioneTesto = v;
    lv_label_set_text(testo, buf);
    lv_obj_scroll_to_y(areaTesto, LV_COORD_MAX, LV_ANIM_OFF);
  }

  if (!s.rtty) {
    char w[8];
    snprintf(w, sizeof(w), "%2d", (int)(s.cwWpm + 0.5f));
    for (char* q = w; *q; q++)
      if (*q == ' ') *q = '_';
    display7::testo(dWpm, w);
    lv_label_set_text(testoWpm, imp.cwWpm ? "WPM · fissa" : "WPM · auto");
    lv_bar_set_value(barraCw, (int32_t)(s.cwLivello * 100), LV_ANIM_OFF);
    lv_obj_set_x(sogliaCw, (lv_coord_t)(s.cwSoglia * lv_obj_get_width(barraCw)) - 1);
    lv_label_set_text(elementiCw, s.cwElementi[0] ? s.cwElementi : " ");
  } else {
    lv_bar_set_value(barraMark, (int32_t)(s.rttyMark * 100), LV_ANIM_OFF);
    lv_bar_set_value(barraSpace, (int32_t)(s.rttySpace * 100), LV_ANIM_OFF);
    if (imp.rttyOpzioni & impostazioni::RTTY_AFC)
      lv_label_set_text_fmt(testoAfc, "AFC: %+d Hz", (int)(s.rttyAfc + (s.rttyAfc >= 0 ? 0.5f : -0.5f)));
    else
      lv_label_set_text(testoAfc, "AFC spento");
  }
}

}  // namespace pag_decoder
