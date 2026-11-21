#include "ui.h"

#include <stdio.h>
#include <time.h>

#include "../config.h"
#include "../display7.h"
#include "../impostazioni.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"

namespace ui {
namespace {

void segnapostoMemorie(lv_obj_t* c) {
  pag_segnaposto::crea(c, "Memorie", "Memorie predefinite (FT8, FT4, WSPR, QRP) e personali.");
}
void segnapostoPropagazione(lv_obj_t* c) {
  pag_segnaposto::crea(c, "Propagazione", "Mappa con grey line, alba e tramonto, indici solari.");
}
void segnapostoDecoder(lv_obj_t* c) {
  pag_segnaposto::crea(c, "Decoder CW / RTTY", "Serve prima l'ingresso audio dalla radio.");
}
void nessunAggiornamento(const link_antenna::Stato&) {}

const Pagina PAGINE[NUM_PAGINE] = {
    {"Sintonia", LV_SYMBOL_GPS, pag_sintonia::crea, pag_sintonia::aggiorna, nullptr},
    {"Memorie", LV_SYMBOL_LIST, segnapostoMemorie, nessunAggiornamento, nullptr},
    {"Propagazione", LV_SYMBOL_WIFI, segnapostoPropagazione, nessunAggiornamento, nullptr},
    {"Decoder", LV_SYMBOL_KEYBOARD, segnapostoDecoder, nessunAggiornamento, nullptr},
    {"Diagnostica", LV_SYMBOL_BARS, pag_diagnostica::crea, pag_diagnostica::aggiorna, nullptr},
    {"Impostazioni", LV_SYMBOL_SETTINGS, pag_impostazioni::crea, pag_impostazioni::aggiorna, nullptr},
    {"Calibrazione", LV_SYMBOL_SETTINGS, pag_calibrazione::crea, pag_calibrazione::aggiorna,
     pag_calibrazione::mostrata},
};

lv_obj_t* pagine[NUM_PAGINE];
lv_obj_t* schede[NUM_SCHEDE];
uint8_t attiva = 0;

lv_obj_t* orologio;
lv_obj_t* puntoAntenna;
lv_obj_t* testoAntenna;
lv_obj_t* chipStato;

volatile bool daRicostruire = false;

// --- Barra in alto ----------------------------------------------------------------------------

void premiStop(lv_event_t*) { link_antenna::invia(proto::Tipo::STOP); }

void creaBarra(lv_obj_t* scr) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();

  lv_obj_t* barra = contenitore(scr, LV_FLEX_FLOW_ROW, 18);
  lv_obj_set_size(barra, cfg::LARGHEZZA, cfg::ALTEZZA_BARRA);
  lv_obj_align(barra, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_flex_align(barra, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_color(barra, t.surface, 0);
  lv_obj_set_style_bg_opa(barra, LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(barra, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(barra, t.border, 0);
  lv_obj_set_style_border_width(barra, 1, 0);
  lv_obj_set_style_pad_left(barra, 20, 0);
  lv_obj_set_style_pad_right(barra, 16, 0);

  etichetta(barra, imp.nominativo[0] ? imp.nominativo : "[NOMINATIVO]", &lv_font_montserrat_22, t.text);
  etichetta(barra, imp.locator[0] ? imp.locator : "[LOCATOR]", &lv_font_montserrat_14, t.text2);

  lv_obj_t* spazio = contenitore(barra, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_grow(spazio, 1);

  lv_obj_t* ora = contenitore(barra, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_flex_align(ora, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  orologio = display7::crea(ora, 26, t.text, t.segOffText);
  display7::testo(orologio, "--:--");
  etichetta(ora, "UTC", &lv_font_montserrat_14, t.text2);

  lv_obj_t* ant = contenitore(barra, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_flex_align(ant, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  puntoAntenna = lv_obj_create(ant);
  lv_obj_remove_style_all(puntoAntenna);
  lv_obj_set_size(puntoAntenna, 12, 12);
  lv_obj_set_style_radius(puntoAntenna, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(puntoAntenna, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(puntoAntenna, t.text2, 0);
  testoAntenna = etichetta(ant, "Antenna", &lv_font_montserrat_16, t.text);

  chipStato = chip(barra, &lv_font_montserrat_14);
  lv_label_set_text(chipStato, "IN ATTESA");
  coloriChip(chipStato, t.surface2, t.text2);

  lv_obj_t* stop = pulsante(barra, LV_SYMBOL_STOP "  STOP", &lv_font_montserrat_20, Stile::PERICOLO);
  lv_obj_set_size(stop, 132, 48);
  lv_obj_set_style_radius(stop, 8, 0);
  lv_obj_add_event_cb(stop, premiStop, LV_EVENT_CLICKED, nullptr);
}

void aggiornaBarra(const link_antenna::Stato& s) {
  const Tema& t = tema::corrente();

  time_t adesso = time(nullptr);
  if (adesso > 1700000000) {  // ora impostata
    struct tm utc;
    gmtime_r(&adesso, &utc);
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", utc.tm_hour, utc.tm_min);
    display7::testo(orologio, buf);
  }

  lv_obj_set_style_bg_color(puntoAntenna, s.collegato ? t.ok : t.danger, 0);
  lv_label_set_text(testoAntenna, !s.peerImpostato ? "Antenna non impostata"
                                  : s.collegato    ? "Antenna"
                                                   : "Antenna persa");

  if (!s.collegato) {
    lv_label_set_text(chipStato, "NON COLLEGATA");
    coloriChip(chipStato, t.dangerBg, t.dangerText);
    return;
  }
  switch (s.r.stato) {
    case proto::Stato::FERMO:
      lv_label_set_text(chipStato, "FERMO");
      coloriChip(chipStato, t.okBg, t.okText);
      break;
    case proto::Stato::IN_MOVIMENTO:
      lv_label_set_text(chipStato, "IN MOVIMENTO");
      coloriChip(chipStato, t.accentBg, t.accent);
      break;
    case proto::Stato::AZZERAMENTO:
      lv_label_set_text(chipStato, "AZZERAMENTO");
      coloriChip(chipStato, t.accentBg, t.accent);
      break;
    case proto::Stato::NON_AZZERATA:
      lv_label_set_text(chipStato, "NON AZZERATA");
      coloriChip(chipStato, t.surface2, t.text2);
      break;
    case proto::Stato::ERRORE:
      lv_label_set_text_fmt(chipStato, "ERRORE: %s", proto::nomeErrore(s.r.errore));
      coloriChip(chipStato, t.dangerBg, t.dangerText);
      break;
  }
}

// --- Schede in basso --------------------------------------------------------------------------

void mostra(uint8_t indice) {
  const Tema& t = tema::corrente();
  if (indice >= NUM_PAGINE) indice = SINTONIA;
  attiva = indice;
  // la Calibrazione sta "dentro" Impostazioni
  uint8_t scheda = indice == CALIBRAZIONE ? (uint8_t)IMPOSTAZIONI : indice;
  for (uint8_t i = 0; i < NUM_PAGINE; i++) {
    if (i == indice) lv_obj_clear_flag(pagine[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(pagine[i], LV_OBJ_FLAG_HIDDEN);
  }
  for (uint8_t i = 0; i < NUM_SCHEDE; i++) {
    bool sel = i == scheda;
    lv_obj_set_style_bg_color(schede[i], sel ? t.surface2 : t.surface, 0);
    lv_obj_set_style_text_color(schede[i], sel ? t.accent : t.text2, 0);
    lv_obj_set_style_border_color(schede[i], sel ? t.accent : t.surface, 0);
  }
  if (PAGINE[indice].mostrata) PAGINE[indice].mostrata();
}

void premiScheda(lv_event_t* e) { mostra((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }

void creaSchede(lv_obj_t* scr) {
  const Tema& t = tema::corrente();
  lv_obj_t* barra = contenitore(scr, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_size(barra, cfg::LARGHEZZA, cfg::ALTEZZA_SCHEDE);
  lv_obj_align(barra, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_bg_color(barra, t.surface, 0);
  lv_obj_set_style_bg_opa(barra, LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(barra, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_color(barra, t.border, 0);
  lv_obj_set_style_border_width(barra, 1, 0);

  for (uint8_t i = 0; i < NUM_SCHEDE; i++) {
    lv_obj_t* b = lv_btn_create(barra);
    lv_obj_remove_style_all(b);
    lv_obj_set_height(b, LV_PCT(100));
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(b, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(b, 3, 0);
    lv_obj_set_style_bg_color(b, t.surface2, LV_STATE_PRESSED);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text_fmt(l, "%s  %s", PAGINE[i].simbolo, PAGINE[i].nome);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, premiScheda, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    schede[i] = b;
  }
}

// --- Costruzione ------------------------------------------------------------------------------

void costruisci() {
  const Tema& t = tema::corrente();
  lv_obj_t* scr = lv_scr_act();
  lv_obj_clean(scr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(scr, t.bg, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(scr, t.text, 0);

  creaBarra(scr);

  for (uint8_t i = 0; i < NUM_PAGINE; i++) {
    lv_obj_t* p = contenitore(scr, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_size(p, cfg::LARGHEZZA, cfg::ALTEZZA - cfg::ALTEZZA_BARRA - cfg::ALTEZZA_SCHEDE);
    lv_obj_align(p, LV_ALIGN_TOP_LEFT, 0, cfg::ALTEZZA_BARRA);
    lv_obj_set_style_pad_hor(p, 20, 0);
    lv_obj_set_style_pad_ver(p, 18, 0);
    PAGINE[i].crea(p);
    pagine[i] = p;
  }

  creaSchede(scr);
  mostra(attiva < NUM_PAGINE ? attiva : 0);
}

void aggiornaTutto(lv_timer_t*) {
  if (daRicostruire) {
    daRicostruire = false;
    tema::imposta(impostazioni::get().tema);
    costruisci();
  }
  link_antenna::Stato s = link_antenna::stato();
  aggiornaBarra(s);
  PAGINE[attiva].aggiorna(s);
}

}  // namespace

void inizia() {
  tema::imposta(impostazioni::get().tema);
  costruisci();
  lv_timer_create(aggiornaTutto, 250, nullptr);
}

void richiediRicostruzione() { daRicostruire = true; }

void vaiA(uint8_t pagina) { mostra(pagina); }

}  // namespace ui
