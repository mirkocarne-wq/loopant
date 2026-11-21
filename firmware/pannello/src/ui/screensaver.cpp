#include "screensaver.h"

#include <bande.h>
#include <stdio.h>

#include "../bandplan.h"
#include "../display7.h"
#include "../impostazioni.h"
#include "../locator.h"
#include "../mappa.h"
#include "../ora.h"
#include "../schermo.h"
#include "../sole.h"
#include "../tema.h"
#include "comuni.h"

namespace screensaver {
namespace {

enum Stile : uint8_t { OROLOGIO = 0, FREQUENZA = 1, GREY_LINE = 2, SPENTO = 3 };

lv_obj_t* radice = nullptr;
uint8_t stile = OROLOGIO;
bool forzato = false;  // anteprima: resta finché non si tocca

lv_obj_t* orologio;
lv_obj_t* etichettaOra;
lv_obj_t* riga2;
lv_obj_t* frequenza;
lv_obj_t* bandaTesto;
lv_obj_t* statoTesto;
lv_obj_t* mappaObj;
time_t ultimaMappa = 0;

lv_color_t fondo() {
  const Tema& t = tema::corrente();
  return tema::indiceCorrente() == 1 ? t.surface : lv_color_mix(lv_color_black(), t.bg, 140);
}

void testoFrequenza(uint32_t hz, char* out, size_t n) {
  if (!hz) {
    snprintf(out, n, "__ ___,_");
    return;
  }
  uint32_t decimi = (hz + 50) / 100, khz = decimi / 10;
  char p[6];
  snprintf(p, sizeof(p), "%5lu", (unsigned long)(khz % 100000));
  for (char* q = p; *q; q++)
    if (*q == ' ') *q = '_';
  snprintf(out, n, "%c%c %c%c%c,%lu", p[0], p[1], p[2], p[3], p[4], (unsigned long)(decimi % 10));
}

void tocco(lv_event_t*) { chiudi(); }

lv_obj_t* contenitorePieno(lv_obj_t* parent, lv_flex_flow_t flusso) {
  lv_obj_t* o = ui::contenitore(parent, flusso, 0);
  lv_obj_set_size(o, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  return o;
}

void creaOrologio(lv_obj_t* r) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  lv_obj_t* c = contenitorePieno(r, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_hor(c, 56, 0);
  lv_obj_set_style_pad_ver(c, 40, 0);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  lv_obj_t* su = ui::contenitore(c, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(su, LV_PCT(100));
  lv_obj_set_flex_align(su, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_t* id = ui::contenitore(su, LV_FLEX_FLOW_COLUMN, 4);
  ui::etichetta(id, imp.nominativo[0] ? imp.nominativo : "[NOMINATIVO]", &font_34, t.text);
  ui::etichetta(id, imp.locator[0] ? imp.locator : "", &font_22, t.text2);
  statoTesto = ui::etichetta(su, "", &font_18, t.text2);

  lv_obj_t* centro = ui::contenitore(c, LV_FLEX_FLOW_COLUMN, 14);
  lv_obj_set_width(centro, LV_PCT(100));
  lv_obj_set_flex_align(centro, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* rr = ui::contenitore(centro, LV_FLEX_FLOW_ROW, 22);
  lv_obj_set_flex_align(rr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  orologio = display7::crea(rr, 160, t.text, t.segOffText);
  display7::testo(orologio, "--:--");
  etichettaOra = ui::etichetta(rr, "UTC", &font_34, t.text2);
  riga2 = ui::etichetta(centro, "", &font_26, t.text2);

  lv_obj_t* giu = ui::contenitore(c, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(giu, LV_PCT(100));
  lv_obj_set_flex_align(giu, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  lv_obj_t* fr = ui::contenitore(giu, LV_FLEX_FLOW_COLUMN, 8);
  ui::titoletto(fr, "ANTENNA ACCORDATA SU · kHz");
  lv_obj_t* fr2 = ui::contenitore(fr, LV_FLEX_FLOW_ROW, 14);
  lv_obj_set_flex_align(fr2, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  frequenza = display7::crea(fr2, 56, t.accent, t.segOffAccent);
  bandaTesto = ui::etichetta(fr2, "", &font_22, t.text2);
  ui::etichetta(giu, "Tocca lo schermo per tornare", &font_18, t.text2);
}

void creaFrequenza(lv_obj_t* r) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  lv_obj_t* c = contenitorePieno(r, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_hor(c, 48, 0);
  lv_obj_set_style_pad_ver(c, 36, 0);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* su = ui::contenitore(c, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(su, LV_PCT(100));
  lv_obj_set_flex_align(su, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  ui::etichetta(su, imp.nominativo[0] ? imp.nominativo : "[NOMINATIVO]", &font_30, t.text);
  lv_obj_t* o = ui::contenitore(su, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  orologio = display7::crea(o, 44, t.text, t.segOffText);
  etichettaOra = ui::etichetta(o, "UTC", &font_20, t.text2);

  lv_obj_t* centro = ui::contenitore(c, LV_FLEX_FLOW_COLUMN, 18);
  lv_obj_set_flex_align(centro, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  ui::titoletto(centro, "ANTENNA ACCORDATA SU · kHz");
  frequenza = display7::crea(centro, 150, t.accent, t.segOffAccent);
  bandaTesto = ui::chip(centro, &font_22);
  ui::coloriChip(bandaTesto, t.surface, t.text);

  lv_obj_t* giu = ui::contenitore(c, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(giu, LV_PCT(100));
  lv_obj_set_flex_align(giu, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  statoTesto = ui::etichetta(giu, "", &font_18, t.text2);
  ui::etichetta(giu, "Tocca lo schermo per tornare", &font_18, t.text2);
  riga2 = nullptr;
}

void creaGreyLine(lv_obj_t* r) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  lv_obj_t* c = contenitorePieno(r, LV_FLEX_FLOW_COLUMN);
  lv_obj_t* su = ui::contenitore(c, LV_FLEX_FLOW_ROW, 24);
  lv_obj_set_size(su, LV_PCT(100), 88);
  lv_obj_set_style_pad_hor(su, 28, 0);
  lv_obj_set_flex_align(su, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  ui::etichetta(su, imp.nominativo[0] ? imp.nominativo : "[NOMINATIVO]", &font_30, t.text);
  riga2 = ui::etichetta(su, "", &font_18, t.text2);
  lv_obj_set_flex_grow(riga2, 1);
  frequenza = display7::crea(su, 36, t.accent, t.segOffAccent);
  lv_obj_t* o = ui::contenitore(su, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  orologio = display7::crea(o, 48, t.text, t.segOffText);
  etichettaOra = ui::etichetta(o, "UTC", &font_18, t.text2);
  mappaObj = mappa::crea(c, 1024, true);
  ultimaMappa = 0;
  statoTesto = nullptr;
  bandaTesto = nullptr;
}

void mostra(bool anteprima) {
  if (radice) return;
  const Impostazioni& imp = impostazioni::get();
  stile = imp.screensaver;
  forzato = anteprima;
  radice = lv_obj_create(lv_layer_sys());
  lv_obj_remove_style_all(radice);
  lv_obj_set_size(radice, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(radice, stile == SPENTO ? lv_color_black() : fondo(), 0);
  lv_obj_set_style_bg_opa(radice, LV_OPA_COVER, 0);
  lv_obj_add_flag(radice, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(radice, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(radice, tocco, LV_EVENT_CLICKED, nullptr);
  orologio = frequenza = bandaTesto = statoTesto = riga2 = etichettaOra = mappaObj = nullptr;
  switch (stile) {
    case OROLOGIO: creaOrologio(radice); break;
    case FREQUENZA: creaFrequenza(radice); break;
    case GREY_LINE: creaGreyLine(radice); break;
    default: schermo::spegni(); break;
  }
}

}  // namespace

bool attivo() { return radice != nullptr; }

void chiudi() {
  if (!radice) return;
  lv_obj_del(radice);
  radice = nullptr;
  if (stile == SPENTO) schermo::accendi();
  lv_disp_trig_activity(nullptr);
}

void anteprima() {
  chiudi();
  mostra(true);
}

void aggiorna(const link_antenna::Stato& s) {
  const Impostazioni& imp = impostazioni::get();
  uint32_t inattivo = lv_disp_get_inactive_time(nullptr);
  if (!radice) {
    if (imp.attesa_min && inattivo > (uint32_t)imp.attesa_min * 60000UL) mostra(false);
    return;
  }
  if (!forzato && inattivo < 500) {  // tocco (anche a schermo spento)
    chiudi();
    return;
  }
  if (stile == SPENTO) return;

  char buf[64], ora1[8], ora2[8];
  bool v = ora::valida();
  ora::hhmmUtc(ora1, sizeof(ora1));
  ora::hhmmLocale(ora2, sizeof(ora2));
  bool locale = imp.modoOra == 1;
  if (orologio) display7::testo(orologio, v ? (locale ? ora2 : ora1) : "--:--");
  if (etichettaOra) lv_label_set_text(etichettaOra, locale ? "locale" : "UTC");

  if (frequenza) {
    testoFrequenza(s.collegato ? s.r.freq_hz : 0, buf, sizeof(buf));
    display7::testo(frequenza, buf);
  }
  if (bandaTesto) {
    const bande::Info* b = bande::info(bande::di(s.r.freq_hz));
    if (s.collegato && b) lv_label_set_text_fmt(bandaTesto, "%s · %s", b->nome, bandplan::segmento(s.r.freq_hz));
    else lv_label_set_text(bandaTesto, "");
  }
  if (statoTesto)
    lv_label_set_text(statoTesto, !s.collegato ? "Antenna non collegata"
                                  : s.r.stato == proto::Stato::FERMO ? "Antenna collegata · ferma"
                                                                      : proto::nomeStato(s.r.stato));

  if (stile == OROLOGIO && riga2) {
    char data[40];
    ora::dataLocale(data, sizeof(data));
    if (v) lv_label_set_text_fmt(riga2, "%s %s · %s", locale ? ora1 : ora2, locale ? "UTC" : "ora locale", data);
    else lv_label_set_text(riga2, "Ora non impostata");
  }

  if (stile == GREY_LINE) {
    double lat = 0, lon = 0;
    bool staz = locator::aCoordinate(imp.locator, lat, lon);
    time_t adesso = time(nullptr);
    if (v && adesso - ultimaMappa >= 60) {
      ultimaMappa = adesso;
      mappa::aggiorna(mappaObj, adesso, staz, lat, lon, imp.nominativo[0] ? imp.nominativo : "Stazione");
    }
    if (riga2) {
      if (v && staz) {
        sole::AlbaTramonto at = sole::albaTramonto(adesso, lat, lon);
        int a = (int)(at.alba * 60 + 0.5), tr = (int)(at.tramonto * 60 + 0.5);
        snprintf(buf, sizeof(buf), "Grey line · alba %02d:%02d · tramonto %02d:%02d UTC", a / 60 % 24, a % 60,
                 tr / 60 % 24, tr % 60);
        lv_label_set_text(riga2, buf);
      } else {
        lv_label_set_text(riga2, v ? "Imposta il locator" : "Ora non impostata");
      }
    }
  }
}

}  // namespace screensaver
