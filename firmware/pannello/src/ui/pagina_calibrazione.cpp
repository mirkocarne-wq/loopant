// Schermata Calibrazione (requisiti K1-K3), aperta da Impostazioni.
// Procedura: docs/software/05-tuning-calibrazione.md
#include <Arduino.h>
#include <bande.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <initializer_list>

#include "../display7.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"
#include "ui.h"

namespace pag_calibrazione {
namespace {

bande::Banda banda = bande::Banda::B40M;
uint32_t versioneTabella = 0xFFFFFFFF;
uint32_t ricaricaAlle = 0;  // millis() a cui rileggere la tabella (0 = no)

lv_obj_t* btn40;
lv_obj_t* btn20;
lv_obj_t* displayPos;
lv_obj_t* testoMm;
lv_obj_t* campoFreq;
lv_obj_t* tastiera = nullptr;
lv_obj_t* grafico;
lv_chart_series_t* serie;
lv_obj_t* asseMin;
lv_obj_t* asseMax;
lv_obj_t* titoloGrafico;
lv_obj_t* elenco;
lv_obj_t* statoTabella;

uint32_t freqRighe[link_antenna::MAX_PUNTI];

const bande::Info* infoBanda() { return bande::info(banda); }

void formattaKhz(uint32_t hz, char* out, size_t n) {
  uint32_t decimi = (hz + 50) / 100;
  uint32_t khz = decimi / 10;
  snprintf(out, n, "%lu %03lu,%lu", (unsigned long)(khz / 1000), (unsigned long)(khz % 1000),
           (unsigned long)(decimi % 10));
}

void formattaPassi(long p, char* out, size_t n) {
  if (p >= 1000) snprintf(out, n, "%ld %03ld", p / 1000, p % 1000);
  else snprintf(out, n, "%ld", p);
}

void ricaricaPresto() { ricaricaAlle = millis() + 400; }

void evidenziaBanda() {
  ui::stilePulsante(btn40, banda == bande::Banda::B40M ? ui::Stile::PRIMARIO : ui::Stile::NORMALE);
  ui::stilePulsante(btn20, banda == bande::Banda::B20M ? ui::Stile::PRIMARIO : ui::Stile::NORMALE);
  char a[16], b[16];
  formattaKhz(infoBanda()->min_hz, a, sizeof(a));
  formattaKhz(infoBanda()->max_hz, b, sizeof(b));
  lv_label_set_text(asseMin, a);
  lv_label_set_text_fmt(asseMax, "%s kHz", b);
  lv_label_set_text_fmt(titoloGrafico, "Posizione ↔ frequenza · %s", infoBanda()->nome);
  // frequenza proposta: centro banda
  uint32_t centro = (infoBanda()->min_hz + infoBanda()->max_hz) / 2;
  char f[16];
  snprintf(f, sizeof(f), "%lu.0", (unsigned long)(centro / 1000));
  lv_textarea_set_text(campoFreq, f);
}

void scegliBanda(lv_event_t* e) {
  banda = lv_event_get_user_data(e) ? bande::Banda::B40M : bande::Banda::B20M;
  evidenziaBanda();
  link_antenna::richiediTabella(banda);
}

void premiJog(lv_event_t* e) {
  link_antenna::invia(proto::Tipo::JOG, 0, (int32_t)(intptr_t)lv_event_get_user_data(e));
}

uint32_t freqCampo() {
  char tmp[16];
  strncpy(tmp, lv_textarea_get_text(campoFreq), sizeof(tmp) - 1);
  tmp[sizeof(tmp) - 1] = 0;
  for (char* p = tmp; *p; p++)
    if (*p == ',') *p = '.';
  double khz = atof(tmp);
  return khz > 0 ? (uint32_t)(khz * 1000.0 + 0.5) : 0;
}

void premiSalva(lv_event_t*) {
  uint32_t hz = freqCampo();
  if (bande::di(hz) != banda) {
    lv_label_set_text(statoTabella, "Frequenza fuori dalla banda scelta");
    return;
  }
  link_antenna::invia(proto::Tipo::CAL_SALVA, hz);
  ricaricaPresto();
}

void premiElimina(lv_event_t* e) {
  uint8_t i = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  link_antenna::invia(proto::Tipo::CAL_ELIMINA, freqRighe[i]);
  ricaricaPresto();
}

void indietro(lv_event_t*) { ui::vaiA(ui::IMPOSTAZIONI); }

void tastieraFatto(lv_event_t*) {
  lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_state(campoFreq, LV_STATE_FOCUSED);
}

void apriTastiera(lv_event_t*) {
  lv_keyboard_set_textarea(tastiera, campoFreq);
  lv_obj_clear_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
}

void creaTastiera() {
  const Tema& t = tema::corrente();
  if (tastiera) lv_obj_del(tastiera);
  tastiera = lv_keyboard_create(lv_layer_top());
  lv_keyboard_set_mode(tastiera, LV_KEYBOARD_MODE_NUMBER);
  lv_obj_set_size(tastiera, 520, 260);
  lv_obj_align(tastiera, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(tastiera, t.surface, 0);
  lv_obj_set_style_bg_color(tastiera, t.surface2, LV_PART_ITEMS);
  lv_obj_set_style_text_color(tastiera, t.text, LV_PART_ITEMS);
  lv_obj_set_style_text_font(tastiera, &font_24, LV_PART_ITEMS);
  lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(tastiera, tastieraFatto, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(tastiera, tastieraFatto, LV_EVENT_CANCEL, nullptr);
}

void ricostruisciTabella(const link_antenna::Tabella& tab) {
  const Tema& t = tema::corrente();
  lv_obj_clean(elenco);
  lv_chart_set_all_value(grafico, serie, LV_CHART_POINT_NONE);
  for (uint16_t i = 0; i < link_antenna::MAX_PUNTI; i++)
    lv_chart_set_value_by_id2(grafico, serie, i, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE);

  uint8_t n = tab.banda == banda ? tab.totale : 0;
  if (!tab.completa && tab.banda == banda) lv_label_set_text(statoTabella, "Lettura dei punti dall'antenna…");
  else if (n == 0) lv_label_set_text(statoTabella, "Nessun punto salvato in questa banda");
  else if (n == 1) lv_label_set_text(statoTabella, "1 punto: ne serve almeno un altro");
  else lv_label_set_text_fmt(statoTabella, "%u punti", n);

  if (n == 0) {
    lv_chart_refresh(grafico);
    return;
  }

  // assi: x in kHz dall'inizio banda, y in centinaia di passi
  int32_t ymin = INT32_MAX, ymax = INT32_MIN;
  for (uint8_t i = 0; i < n; i++) {
    int32_t y = tab.posizione[i] / 100;
    if (y < ymin) ymin = y;
    if (y > ymax) ymax = y;
  }
  int32_t margine = (ymax - ymin) / 6 + 20;
  lv_chart_set_range(grafico, LV_CHART_AXIS_PRIMARY_X, 0,
                     (lv_coord_t)((infoBanda()->max_hz - infoBanda()->min_hz) / 1000));
  lv_chart_set_range(grafico, LV_CHART_AXIS_PRIMARY_Y, (lv_coord_t)(ymin - margine), (lv_coord_t)(ymax + margine));

  for (uint8_t i = 0; i < n; i++) {
    if (!tab.freq_hz[i] && !tab.posizione[i]) continue;  // non ancora letto
    lv_coord_t x = (lv_coord_t)((tab.freq_hz[i] - infoBanda()->min_hz) / 1000);
    lv_chart_set_value_by_id2(grafico, serie, i, x, (lv_coord_t)(tab.posizione[i] / 100));

    freqRighe[i] = tab.freq_hz[i];
    lv_obj_t* r = ui::contenitore(elenco, LV_FLEX_FLOW_ROW, 8);
    lv_obj_set_width(r, LV_PCT(100));
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    char a[16], b[16];
    formattaKhz(tab.freq_hz[i], a, sizeof(a));
    formattaPassi((long)tab.posizione[i], b, sizeof(b));
    lv_obj_t* lf = ui::etichetta(r, "", &font_18, t.text);
    lv_label_set_text_fmt(lf, "%s kHz", a);
    lv_obj_set_flex_grow(lf, 1);
    lv_obj_t* lp = ui::etichetta(r, b, &font_18, t.text2);
    lv_obj_set_width(lp, 110);
    lv_obj_t* del = ui::pulsante(r, LV_SYMBOL_TRASH, &font_18, ui::Stile::SCHEDA);
    lv_obj_set_size(del, 48, 44);
    lv_obj_set_style_text_color(del, t.dangerText, 0);
    lv_obj_set_style_border_width(del, 0, 0);
    lv_obj_add_event_cb(del, premiElimina, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  lv_chart_refresh(grafico);
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 12, 0);
  lv_obj_set_style_pad_top(c, 12, 0);

  // Intestazione
  lv_obj_t* testa = ui::contenitore(c, LV_FLEX_FLOW_ROW, 14);
  lv_obj_set_width(testa, LV_PCT(100));
  lv_obj_set_flex_align(testa, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* back = ui::pulsante(testa, LV_SYMBOL_LEFT " Impostazioni", &font_16, ui::Stile::NORMALE);
  lv_obj_set_size(back, 160, 44);
  lv_obj_add_event_cb(back, indietro, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* titolo = ui::etichetta(testa, "Calibrazione antenna", &font_22, t.text);
  lv_obj_set_flex_grow(titolo, 1);
  btn40 = ui::pulsante(testa, "40 m", &font_18, ui::Stile::NORMALE);
  btn20 = ui::pulsante(testa, "20 m", &font_18, ui::Stile::NORMALE);
  for (lv_obj_t* b : {btn40, btn20}) lv_obj_set_size(b, 110, 44);
  lv_obj_add_event_cb(btn40, scegliBanda, LV_EVENT_CLICKED, (void*)1);
  lv_obj_add_event_cb(btn20, scegliBanda, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* corpo = ui::contenitore(c, LV_FLEX_FLOW_ROW, 16);
  lv_obj_set_width(corpo, LV_PCT(100));
  lv_obj_set_flex_grow(corpo, 1);

  // Colonna sinistra: posizione, jog, salvataggio
  lv_obj_t* sx = ui::contenitore(corpo, LV_FLEX_FLOW_COLUMN, 12);
  lv_obj_set_size(sx, 420, LV_PCT(100));

  lv_obj_t* pos = ui::scheda(sx);
  lv_obj_set_width(pos, LV_PCT(100));
  ui::titoletto(pos, "POSIZIONE MOTORE · PASSI");
  displayPos = display7::crea(pos, 48, t.accent, t.segOffAccent);
  display7::testo(displayPos, "___ ___");
  testoMm = ui::etichetta(pos, "Arrivo sempre dal verso crescente", &font_14, t.text2);

  lv_obj_t* jog = ui::contenitore(sx, LV_FLEX_FLOW_ROW_WRAP, 8);
  lv_obj_set_width(jog, LV_PCT(100));
  const int passi[] = {-1000, -100, -10, 10, 100, 1000};
  const char* nomi[] = {"−1000", "−100", "−10", "+10", "+100", "+1000"};
  for (int i = 0; i < 6; i++) {
    lv_obj_t* b = ui::pulsante(jog, nomi[i], &font_22, ui::Stile::NORMALE);
    lv_obj_set_size(b, 134, 54);
    lv_obj_add_event_cb(b, premiJog, LV_EVENT_CLICKED, (void*)(intptr_t)passi[i]);
  }

  ui::etichetta(sx, "Frequenza del dip sul NanoVNA (kHz)", &font_14, t.text2);
  lv_obj_t* salva = ui::contenitore(sx, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(salva, LV_PCT(100));
  campoFreq = lv_textarea_create(salva);
  lv_textarea_set_one_line(campoFreq, true);
  lv_textarea_set_accepted_chars(campoFreq, "0123456789.,");
  lv_textarea_set_max_length(campoFreq, 9);
  lv_obj_set_height(campoFreq, 54);
  lv_obj_set_flex_grow(campoFreq, 1);
  lv_obj_set_style_bg_color(campoFreq, t.bg, 0);
  lv_obj_set_style_border_color(campoFreq, t.borderStrong, 0);
  lv_obj_set_style_border_color(campoFreq, t.accent, LV_STATE_FOCUSED);
  lv_obj_set_style_border_width(campoFreq, 2, 0);
  lv_obj_set_style_radius(campoFreq, 10, 0);
  lv_obj_set_style_text_color(campoFreq, t.text, 0);
  lv_obj_set_style_text_font(campoFreq, &font_24, 0);
  lv_obj_add_event_cb(campoFreq, apriTastiera, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* bs = ui::pulsante(salva, "Salva punto", &font_18, ui::Stile::PRIMARIO);
  lv_obj_set_size(bs, 150, 54);
  lv_obj_add_event_cb(bs, premiSalva, LV_EVENT_CLICKED, nullptr);

  // Colonna destra: grafico e punti
  lv_obj_t* dx = ui::contenitore(corpo, LV_FLEX_FLOW_COLUMN, 12);
  lv_obj_set_height(dx, LV_PCT(100));
  lv_obj_set_flex_grow(dx, 1);

  lv_obj_t* gr = ui::scheda(dx);
  lv_obj_set_width(gr, LV_PCT(100));
  lv_obj_set_style_pad_row(gr, 4, 0);
  titoloGrafico = ui::etichetta(gr, "", &font_16, t.text);
  grafico = lv_chart_create(gr);
  lv_obj_set_size(grafico, LV_PCT(100), 110);
  lv_chart_set_type(grafico, LV_CHART_TYPE_SCATTER);
  lv_chart_set_point_count(grafico, link_antenna::MAX_PUNTI);
  lv_chart_set_div_line_count(grafico, 3, 4);
  lv_obj_set_style_bg_color(grafico, t.surface, 0);
  lv_obj_set_style_border_color(grafico, t.border, 0);
  lv_obj_set_style_line_color(grafico, t.border, LV_PART_MAIN);
  lv_obj_set_style_line_width(grafico, 3, LV_PART_ITEMS);
  lv_obj_set_style_size(grafico, 10, LV_PART_INDICATOR);
  serie = lv_chart_add_series(grafico, t.accent, LV_CHART_AXIS_PRIMARY_Y);
  lv_obj_t* assi = ui::contenitore(gr, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(assi, LV_PCT(100));
  lv_obj_set_flex_align(assi, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  asseMin = ui::etichetta(assi, "", &font_12, t.text2);
  asseMax = ui::etichetta(assi, "", &font_12, t.text2);

  lv_obj_t* tb = ui::scheda(dx);
  lv_obj_set_width(tb, LV_PCT(100));
  lv_obj_set_flex_grow(tb, 1);
  lv_obj_set_style_pad_row(tb, 4, 0);
  statoTabella = ui::etichetta(tb, "", &font_14, t.text2);
  elenco = ui::contenitore(tb, LV_FLEX_FLOW_COLUMN, 2);
  lv_obj_set_width(elenco, LV_PCT(100));
  lv_obj_set_flex_grow(elenco, 1);
  lv_obj_add_flag(elenco, LV_OBJ_FLAG_SCROLLABLE);

  creaTastiera();
  evidenziaBanda();
  versioneTabella = 0xFFFFFFFF;
}

void mostrata() { link_antenna::richiediTabella(banda); }

void aggiorna(const link_antenna::Stato& s) {
  char buf[16];
  if (s.ricevuta) {
    long p = (long)s.r.posizione;
    if (p < 0) p = 0;
    char cifre[8];
    snprintf(cifre, sizeof(cifre), "%6ld", p % 1000000);
    for (char* q = cifre; *q; q++)
      if (*q == ' ') *q = '_';
    snprintf(buf, sizeof(buf), "%.3s %.3s", cifre, cifre + 3);
    display7::testo(displayPos, buf);
    if (!(s.r.flag & proto::FLAG_AZZERATA))
      lv_label_set_text(testoMm, "Antenna non azzerata: premere ZERO prima del jog");
    else
      lv_label_set_text(testoMm, "Arrivo sempre dal verso crescente");
  }

  if (ricaricaAlle && (int32_t)(millis() - ricaricaAlle) >= 0) {
    ricaricaAlle = 0;
    link_antenna::richiediTabella(banda);
  }
  link_antenna::Tabella tab = link_antenna::tabella();
  if (tab.versione != versioneTabella) {
    versioneTabella = tab.versione;
    ricostruisciTabella(tab);
  }
}

}  // namespace pag_calibrazione
