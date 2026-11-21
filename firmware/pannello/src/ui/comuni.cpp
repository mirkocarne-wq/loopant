#include "comuni.h"

namespace ui {

lv_obj_t* contenitore(lv_obj_t* parent, lv_flex_flow_t flusso, lv_coord_t spazio) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(o, flusso);
  lv_obj_set_style_pad_row(o, spazio, 0);
  lv_obj_set_style_pad_column(o, spazio, 0);
  lv_obj_set_size(o, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  return o;
}

lv_obj_t* scheda(lv_obj_t* parent) {
  const Tema& t = tema::corrente();
  lv_obj_t* o = contenitore(parent, LV_FLEX_FLOW_COLUMN, 8);
  lv_obj_set_style_bg_color(o, t.surface, 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(o, t.border, 0);
  lv_obj_set_style_border_width(o, 1, 0);
  lv_obj_set_style_radius(o, 12, 0);
  lv_obj_set_style_pad_hor(o, 18, 0);
  lv_obj_set_style_pad_ver(o, 14, 0);
  return o;
}

lv_obj_t* etichetta(lv_obj_t* parent, const char* testo, const lv_font_t* font, lv_color_t colore) {
  lv_obj_t* l = lv_label_create(parent);
  lv_label_set_text(l, testo);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, colore, 0);
  return l;
}

lv_obj_t* titoletto(lv_obj_t* parent, const char* testo) {
  lv_obj_t* l = etichetta(parent, testo, &font_14, tema::corrente().text2);
  lv_obj_set_style_text_letter_space(l, 1, 0);
  return l;
}

void stilePulsante(lv_obj_t* b, Stile stile) {
  const Tema& t = tema::corrente();
  lv_color_t fondo = t.surface2, testo = t.text, bordo = t.border;
  lv_coord_t spessore = 1;
  switch (stile) {
    case Stile::NORMALE: break;
    case Stile::SCHEDA: fondo = t.surface; break;
    case Stile::PRIMARIO: fondo = t.accent; testo = t.onAccent; bordo = t.accent; spessore = 0; break;
    case Stile::ATTIVO: fondo = t.accentBg; testo = t.accent; bordo = t.accent; spessore = 2; break;
    case Stile::PERICOLO: fondo = t.danger; testo = t.onDanger; bordo = t.danger; spessore = 0; break;
  }
  lv_obj_set_style_bg_color(b, fondo, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(b, testo, 0);
  lv_obj_set_style_border_color(b, bordo, 0);
  lv_obj_set_style_border_width(b, spessore, 0);
  lv_obj_set_style_bg_color(b, lv_color_mix(testo, fondo, 60), LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(b, t.surface, LV_STATE_DISABLED);
  lv_obj_set_style_text_color(b, t.text2, LV_STATE_DISABLED);
  lv_obj_set_style_border_color(b, t.border, LV_STATE_DISABLED);
  lv_obj_set_style_border_width(b, 2, LV_STATE_DISABLED);
}

lv_obj_t* pulsante(lv_obj_t* parent, const char* testo, const lv_font_t* font, Stile stile) {
  lv_obj_t* b = lv_btn_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_style_radius(b, 10, 0);
  stilePulsante(b, stile);
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, testo);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_center(l);
  return b;
}

lv_obj_t* chip(lv_obj_t* parent, const lv_font_t* font) {
  lv_obj_t* l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_bg_opa(l, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(l, 6, 0);
  lv_obj_set_style_pad_hor(l, 12, 0);
  lv_obj_set_style_pad_ver(l, 6, 0);
  return l;
}

void coloriChip(lv_obj_t* c, lv_color_t fondo, lv_color_t testo) {
  lv_obj_set_style_bg_color(c, fondo, 0);
  lv_obj_set_style_text_color(c, testo, 0);
}

}  // namespace ui
