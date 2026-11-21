// Schermata Impostazioni (requisiti I1-I5). Prima versione: stazione, tema, antenna.
// Screensaver, luminosità e ora: da aggiungere.
#include <collegamento.h>
#include <ctype.h>
#include <string.h>

#include "../impostazioni.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"
#include "ui.h"

namespace pag_impostazioni {
namespace {

lv_obj_t* tastiera = nullptr;
lv_obj_t* campoNominativo;
lv_obj_t* campoLocator;
lv_obj_t* testoMac;

void maiuscolo(char* s) {
  for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

void conferma(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* ta = lv_keyboard_get_textarea(tastiera);
  if (code == LV_EVENT_READY && ta) {
    Impostazioni& imp = impostazioni::get();
    if (ta == campoNominativo) {
      strncpy(imp.nominativo, lv_textarea_get_text(ta), sizeof(imp.nominativo) - 1);
      maiuscolo(imp.nominativo);
    } else if (ta == campoLocator) {
      strncpy(imp.locator, lv_textarea_get_text(ta), sizeof(imp.locator) - 1);
      maiuscolo(imp.locator);
    }
    impostazioni::salva();
    ui::richiediRicostruzione();  // il nominativo compare nella barra in alto
  }
  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
    if (ta) lv_obj_clear_state(ta, LV_STATE_FOCUSED);
  }
}

void apriTastiera(lv_event_t* e) {
  lv_obj_t* ta = lv_event_get_target(e);
  lv_keyboard_set_textarea(tastiera, ta);
  lv_keyboard_set_mode(tastiera, LV_KEYBOARD_MODE_TEXT_UPPER);
  lv_obj_clear_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t* campo(lv_obj_t* parent, const char* nome, const char* valore, uint32_t max, lv_coord_t larghezza) {
  const Tema& t = tema::corrente();
  lv_obj_t* box = ui::contenitore(parent, LV_FLEX_FLOW_COLUMN, 6);
  ui::etichetta(box, nome, &font_14, t.text2);
  lv_obj_t* ta = lv_textarea_create(box);
  lv_textarea_set_one_line(ta, true);
  lv_textarea_set_max_length(ta, max);
  lv_textarea_set_text(ta, valore);
  lv_obj_set_size(ta, larghezza, 52);
  lv_obj_set_style_bg_color(ta, t.bg, 0);
  lv_obj_set_style_border_color(ta, t.borderStrong, 0);
  lv_obj_set_style_border_color(ta, t.accent, LV_STATE_FOCUSED);
  lv_obj_set_style_border_width(ta, 2, 0);
  lv_obj_set_style_radius(ta, 10, 0);
  lv_obj_set_style_text_color(ta, t.text, 0);
  lv_obj_set_style_text_font(ta, &font_22, 0);
  lv_obj_add_event_cb(ta, apriTastiera, LV_EVENT_CLICKED, nullptr);
  return ta;
}

void apriCalibrazione(lv_event_t*) { ui::vaiA(ui::CALIBRAZIONE); }

void scegliTema(lv_event_t* e) {
  uint8_t i = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  impostazioni::get().tema = i;
  impostazioni::salva();
  ui::richiediRicostruzione();
}

void creaTastiera() {
  const Tema& t = tema::corrente();
  if (tastiera) lv_obj_del(tastiera);
  tastiera = lv_keyboard_create(lv_layer_top());
  lv_obj_set_size(tastiera, LV_PCT(100), 280);
  lv_obj_align(tastiera, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(tastiera, t.surface, 0);
  lv_obj_set_style_bg_color(tastiera, t.surface2, LV_PART_ITEMS);
  lv_obj_set_style_text_color(tastiera, t.text, LV_PART_ITEMS);
  lv_obj_set_style_text_font(tastiera, &font_20, LV_PART_ITEMS);
  lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(tastiera, conferma, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(tastiera, conferma, LV_EVENT_CANCEL, nullptr);
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  lv_obj_set_style_pad_column(c, 16, 0);

  lv_obj_t* sx = ui::contenitore(c, LV_FLEX_FLOW_COLUMN, 12);
  lv_obj_set_flex_grow(sx, 1);
  lv_obj_t* dx = ui::contenitore(c, LV_FLEX_FLOW_COLUMN, 12);
  lv_obj_set_flex_grow(dx, 1);

  // Stazione
  lv_obj_t* st = ui::scheda(sx);
  lv_obj_set_width(st, LV_PCT(100));
  ui::titoletto(st, "STAZIONE");
  lv_obj_t* riga = ui::contenitore(st, LV_FLEX_FLOW_ROW, 10);
  campoNominativo = campo(riga, "Nominativo", imp.nominativo, sizeof(imp.nominativo) - 1, 250);
  campoLocator = campo(riga, "Locator", imp.locator, sizeof(imp.locator) - 1, 170);

  // Antenna
  lv_obj_t* an = ui::scheda(sx);
  lv_obj_set_width(an, LV_PCT(100));
  ui::titoletto(an, "ANTENNA");
  char mac[18];
  Collegamento::scriviMac(imp.peer, mac);
  testoMac = ui::etichetta(an, "", &font_18, t.text);
  lv_label_set_text_fmt(testoMac, "MAC dell'antenna: %s", mac);
  uint8_t mio[6];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, mac);
  lv_obj_t* l = ui::etichetta(an, "", &font_14, t.text2);
  lv_label_set_text_fmt(l, "MAC di questo pannello: %s\nPer ora il MAC dell'antenna si imposta dalla console: mac AA:BB:...", mac);
  lv_obj_t* cal = ui::pulsante(an, "Calibrazione antenna  " LV_SYMBOL_RIGHT, &font_18, ui::Stile::NORMALE);
  lv_obj_set_size(cal, LV_PCT(100), 52);
  lv_obj_add_event_cb(cal, apriCalibrazione, LV_EVENT_CLICKED, nullptr);

  // Tema
  lv_obj_t* te = ui::scheda(dx);
  lv_obj_set_width(te, LV_PCT(100));
  ui::titoletto(te, "TEMA");
  lv_obj_t* griglia = ui::contenitore(te, LV_FLEX_FLOW_ROW_WRAP, 10);
  lv_obj_set_width(griglia, LV_PCT(100));
  for (uint8_t i = 0; i < tema::NUMERO; i++) {
    const Tema& x = tema::di(i);
    bool sel = i == tema::indiceCorrente();
    lv_obj_t* b = lv_btn_create(griglia);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, 212, 76);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(b, x.bg, 0);
    lv_obj_set_style_border_color(b, sel ? t.accent : t.border, 0);
    lv_obj_set_style_border_width(b, sel ? 3 : 1, 0);
    lv_obj_t* n = ui::etichetta(b, x.nome, &font_16, x.text);
    lv_obj_align(n, LV_ALIGN_BOTTOM_LEFT, 12, -10);
    lv_obj_t* f = ui::etichetta(b, "7 074", &font_26, x.accent);
    lv_obj_align(f, LV_ALIGN_TOP_LEFT, 12, 8);
    lv_obj_add_event_cb(b, scegliTema, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }

  creaTastiera();
}

void aggiorna(const link_antenna::Stato&) {}

}  // namespace pag_impostazioni

namespace pag_segnaposto {

void crea(lv_obj_t* c, const char* nome, const char* descrizione) {
  const Tema& t = tema::corrente();
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(c, 12, 0);
  ui::etichetta(c, nome, &font_30, t.text);
  ui::etichetta(c, descrizione, &font_18, t.text2);
  ui::etichetta(c, "Schermata in preparazione", &font_16, t.accent);
}

}  // namespace pag_segnaposto
