// Schermata Sintonia (requisiti S1-S6, docs/software/03-unita-controllo.md).
#include <bande.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <initializer_list>

#include "../bandplan.h"
#include "../display7.h"
#include "../impostazioni.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"

namespace pag_sintonia {
namespace {

char ingresso[10] = "7074,0";  // kHz con virgola, come digitato
bool fresco = true;            // il prossimo tasto ricomincia da capo
bool collegato = false;

lv_obj_t* display;
lv_obj_t* chipBanda;
lv_obj_t* testoAccordata;
lv_obj_t* testoPosizione;
lv_obj_t* btn40;
lv_obj_t* btn20;
lv_obj_t* btnTune;
lv_obj_t* btnZero;

const char* TASTI[] = {"7", "8", "9", "\n", "4", "5", "6", "\n", "1", "2", "3", "\n",
                       ",", "0", LV_SYMBOL_BACKSPACE, ""};

uint32_t frequenzaHz() {
  char tmp[10];
  strncpy(tmp, ingresso, sizeof(tmp));
  tmp[sizeof(tmp) - 1] = 0;
  for (char* p = tmp; *p; p++)
    if (*p == ',') *p = '.';
  double khz = atof(tmp);
  return khz > 0 ? (uint32_t)(khz * 1000.0 + 0.5) : 0;
}


// "7074,0" -> "_7 074,0" (5 cifre intere, le vuote spente)
void testoDisplay(char* out, size_t n) {
  char intero[8] = "", dec[3] = "_";
  const char* virgola = strchr(ingresso, ',');
  size_t li = virgola ? (size_t)(virgola - ingresso) : strlen(ingresso);
  if (li > 5) li = 5;
  memcpy(intero, ingresso, li);
  intero[li] = 0;
  if (virgola && virgola[1]) {
    dec[0] = virgola[1];
    dec[1] = 0;
  }
  char p[6];
  size_t pad = 5 - strlen(intero);
  for (size_t i = 0; i < pad; i++) p[i] = '_';
  strcpy(p + pad, intero);
  snprintf(out, n, "%c%c %c%c%c,%s", p[0], p[1], p[2], p[3], p[4], dec);
}

void formattaKhz(uint32_t hz, char* out, size_t n) {
  uint32_t decimi = (hz + 50) / 100;
  uint32_t khz = decimi / 10;
  if (khz >= 1000)
    snprintf(out, n, "%lu %03lu,%lu", (unsigned long)(khz / 1000), (unsigned long)(khz % 1000),
             (unsigned long)(decimi % 10));
  else
    snprintf(out, n, "%lu,%lu", (unsigned long)khz, (unsigned long)(decimi % 10));
}

void aggiornaIngresso() {
  const Tema& t = tema::corrente();
  char buf[16];
  testoDisplay(buf, sizeof(buf));
  display7::testo(display, buf);

  uint32_t hz = frequenzaHz();
  bande::Banda b = bande::di(hz);
  bool valida = b != bande::Banda::NESSUNA;
  if (valida) {
    lv_label_set_text_fmt(chipBanda, "%s · %s", bande::info(b)->nome, bandplan::segmento(hz));
    ui::coloriChip(chipBanda, t.surface2, t.text);
  } else {
    lv_label_set_text(chipBanda, "Fuori banda: 7 000–7 200 o 14 000–14 350 kHz");
    ui::coloriChip(chipBanda, t.dangerBg, t.dangerText);
  }
  ui::stilePulsante(btn40, b == bande::Banda::B40M ? ui::Stile::ATTIVO : ui::Stile::NORMALE);
  ui::stilePulsante(btn20, b == bande::Banda::B20M ? ui::Stile::ATTIVO : ui::Stile::NORMALE);

  if (collegato) lv_obj_clear_state(btnZero, LV_STATE_DISABLED);
  else lv_obj_add_state(btnZero, LV_STATE_DISABLED);

  lv_obj_t* l = lv_obj_get_child(btnTune, 0);
  if (valida && collegato) {
    lv_obj_clear_state(btnTune, LV_STATE_DISABLED);
    lv_label_set_text(l, "TUNE");
  } else {
    lv_obj_add_state(btnTune, LV_STATE_DISABLED);
    lv_label_set_text(l, !collegato ? "Antenna non collegata" : "TUNE non disponibile");
  }
}

void imposta(uint32_t hz) {
  uint32_t decimi = (hz + 50) / 100;
  snprintf(ingresso, sizeof(ingresso), "%lu,%lu", (unsigned long)(decimi / 10),
           (unsigned long)(decimi % 10));
  fresco = true;
  aggiornaIngresso();
}

void premiTasto(lv_event_t* e) {
  lv_obj_t* bm = lv_event_get_target(e);
  uint16_t id = lv_btnmatrix_get_selected_btn(bm);
  const char* k = lv_btnmatrix_get_btn_text(bm, id);
  if (!k) return;
  if (fresco) {
    ingresso[0] = 0;
    fresco = false;
  }
  size_t n = strlen(ingresso);
  char* virgola = strchr(ingresso, ',');
  if (!strcmp(k, LV_SYMBOL_BACKSPACE)) {
    if (n) ingresso[n - 1] = 0;
  } else if (!strcmp(k, ",")) {
    if (!virgola && n < sizeof(ingresso) - 2) {
      if (!n) strcpy(ingresso, "0");
      strcat(ingresso, ",");
    }
  } else {
    if (virgola && strlen(virgola + 1) >= 1) return;  // un solo decimale
    if (!virgola && n >= 5) return;                   // massimo 5 cifre intere
    if (n < sizeof(ingresso) - 1) {
      ingresso[n] = k[0];
      ingresso[n + 1] = 0;
    }
  }
  aggiornaIngresso();
}

void premiBanda(lv_event_t* e) {
  bool b40 = lv_event_get_user_data(e) != nullptr;
  const Impostazioni& imp = impostazioni::get();
  imposta(b40 ? imp.ultima40_hz : imp.ultima20_hz);
}

void premiPasso(lv_event_t* e) {
  int32_t passo = (int32_t)(intptr_t)lv_event_get_user_data(e) * 1000;
  int32_t hz = (int32_t)frequenzaHz() + passo;
  imposta(hz > 0 ? (uint32_t)hz : 0);
}

void premiTune(lv_event_t*) {
  uint32_t hz = frequenzaHz();
  bande::Banda b = bande::di(hz);
  if (b == bande::Banda::NESSUNA) return;
  link_antenna::invia(proto::Tipo::GOTO, hz);
  Impostazioni& imp = impostazioni::get();
  if (b == bande::Banda::B40M) imp.ultima40_hz = hz;
  else imp.ultima20_hz = hz;
  impostazioni::salva();
  fresco = true;
}

// Azzeramento sul finecorsa (anche il primo TUNE di ogni banda lo fa da solo)
void premiZero(lv_event_t*) { link_antenna::invia(proto::Tipo::ZERO); }

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  lv_obj_set_style_pad_column(c, 20, 0);

  // --- colonna sinistra ---
  lv_obj_t* sx = ui::contenitore(c, LV_FLEX_FLOW_COLUMN, 12);
  lv_obj_set_height(sx, LV_PCT(100));
  lv_obj_set_flex_grow(sx, 1);

  lv_obj_t* card = ui::scheda(sx);
  lv_obj_set_width(card, LV_PCT(100));
  lv_obj_set_style_pad_row(card, 10, 0);
  ui::titoletto(card, "FREQUENZA DA ACCORDARE");
  lv_obj_t* riga = ui::contenitore(card, LV_FLEX_FLOW_ROW, 14);
  lv_obj_set_flex_align(riga, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  display = display7::crea(riga, 84, t.accent, t.segOffAccent);
  ui::etichetta(riga, "kHz", &font_26, t.text2);
  chipBanda = ui::chip(card, &font_16);

  lv_obj_t* info = ui::contenitore(sx, LV_FLEX_FLOW_ROW, 12);
  lv_obj_set_width(info, LV_PCT(100));
  lv_obj_set_flex_align(info, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(info, 4, 0);
  testoAccordata = ui::etichetta(info, "Antenna accordata su —", &font_16, t.text2);
  testoPosizione = ui::etichetta(info, "Posizione —", &font_16, t.text2);

  lv_obj_t* tasti = ui::contenitore(sx, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(tasti, LV_PCT(100));
  btn40 = ui::pulsante(tasti, "40 m", &font_22, ui::Stile::NORMALE);
  btn20 = ui::pulsante(tasti, "20 m", &font_22, ui::Stile::NORMALE);
  for (lv_obj_t* b : {btn40, btn20}) {
    lv_obj_set_height(b, 62);
    lv_obj_set_flex_grow(b, 1);
  }
  lv_obj_add_event_cb(btn40, premiBanda, LV_EVENT_CLICKED, (void*)1);
  lv_obj_add_event_cb(btn20, premiBanda, LV_EVENT_CLICKED, nullptr);
  const int passi[] = {-5, -1, 1, 5};
  const char* etichettePassi[] = {"−5", "−1", "+1", "+5"};
  for (int i = 0; i < 4; i++) {
    lv_obj_t* b = ui::pulsante(tasti, etichettePassi[i], &font_20, ui::Stile::SCHEDA);
    lv_obj_set_size(b, 80, 62);
    lv_obj_add_event_cb(b, premiPasso, LV_EVENT_CLICKED, (void*)(intptr_t)passi[i]);
  }

  lv_obj_t* filaTune = ui::contenitore(sx, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(filaTune, LV_PCT(100));
  lv_obj_set_flex_grow(filaTune, 1);
  btnTune = ui::pulsante(filaTune, "TUNE", &font_30, ui::Stile::PRIMARIO);
  lv_obj_set_height(btnTune, LV_PCT(100));
  lv_obj_set_flex_grow(btnTune, 1);
  lv_obj_set_style_radius(btnTune, 12, 0);
  lv_obj_set_style_text_letter_space(lv_obj_get_child(btnTune, 0), 4, 0);
  lv_obj_add_event_cb(btnTune, premiTune, LV_EVENT_CLICKED, nullptr);
  btnZero = ui::pulsante(filaTune, "ZERO", &font_22, ui::Stile::NORMALE);
  lv_obj_set_size(btnZero, 130, LV_PCT(100));
  lv_obj_set_style_radius(btnZero, 12, 0);
  lv_obj_add_event_cb(btnZero, premiZero, LV_EVENT_CLICKED, nullptr);

  // --- tastierino ---
  lv_obj_t* bm = lv_btnmatrix_create(c);
  lv_btnmatrix_set_map(bm, TASTI);
  lv_obj_set_size(bm, 340, LV_PCT(100));
  lv_obj_set_style_bg_opa(bm, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bm, 0, 0);
  lv_obj_set_style_pad_all(bm, 0, 0);
  lv_obj_set_style_pad_row(bm, 10, 0);
  lv_obj_set_style_pad_column(bm, 10, 0);
  lv_obj_set_style_bg_color(bm, t.surface2, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(bm, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_border_color(bm, t.border, LV_PART_ITEMS);
  lv_obj_set_style_border_width(bm, 1, LV_PART_ITEMS);
  lv_obj_set_style_radius(bm, 12, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(bm, 0, LV_PART_ITEMS);
  lv_obj_set_style_text_color(bm, t.text, LV_PART_ITEMS);
  lv_obj_set_style_text_font(bm, &font_34, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(bm, lv_color_mix(t.text, t.surface2, 60), LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_add_event_cb(bm, premiTasto, LV_EVENT_VALUE_CHANGED, nullptr);

  aggiornaIngresso();
}

void aggiorna(const link_antenna::Stato& s) {
  const Tema& t = tema::corrente();
  if (s.collegato != collegato) {
    collegato = s.collegato;
    aggiornaIngresso();
  }
  char buf[24];
  if (s.collegato && s.r.freq_hz) {
    formattaKhz(s.r.freq_hz, buf, sizeof(buf));
    lv_label_set_text_fmt(testoAccordata, "Antenna accordata su #%06lx %s kHz#",
                          (unsigned long)(lv_color_to32(t.text) & 0xFFFFFF), buf);
    lv_label_set_recolor(testoAccordata, true);
  } else {
    lv_label_set_text(testoAccordata, "Antenna accordata su —");
  }
  if (s.collegato) {
    long p = (long)s.r.posizione;
    if (p >= 1000) snprintf(buf, sizeof(buf), "%ld %03ld", p / 1000, p % 1000);
    else snprintf(buf, sizeof(buf), "%ld", p);
    lv_label_set_text_fmt(testoPosizione, "Posizione #%06lx %s#",
                          (unsigned long)(lv_color_to32(t.text) & 0xFFFFFF), buf);
    lv_label_set_recolor(testoPosizione, true);
  } else {
    lv_label_set_text(testoPosizione, "Posizione —");
  }
}

void proponi(uint32_t hz) { imposta(hz); }

uint32_t frequenzaDigitata() { return frequenzaHz(); }

}  // namespace pag_sintonia
