// Schermata Diagnostica (requisiti G1-G5, docs/software/03-unita-controllo.md).
#include <Arduino.h>
#include <stdio.h>

#include <initializer_list>

#include <Grafica.h>
#include "../eventi.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"

namespace pag_diagnostica {
namespace {

lv_obj_t* puntoColl;
lv_obj_t* testoColl;
lv_obj_t* barraSegnale;
lv_obj_t* valSegnale;
lv_obj_t* valRipetuti;
lv_obj_t* valUltima;

lv_obj_t* displayPos;
lv_obj_t* valDriver;
lv_obj_t* chipFcMin;
lv_obj_t* chipFcMax;
lv_obj_t* valAzzerata;

lv_obj_t* valMovimenti;
lv_obj_t* valAzzeramenti;
lv_obj_t* valAcceso;
lv_obj_t* valVersione;

lv_obj_t* righeEventi[eventi::MAX_EVENTI];
lv_obj_t* chipEventi[eventi::MAX_EVENTI];
lv_obj_t* testiEventi[eventi::MAX_EVENTI];
lv_obj_t* oreEventi[eventi::MAX_EVENTI];
lv_obj_t* nessunEvento;

// Riga "etichetta ........ valore"
lv_obj_t* riga(lv_obj_t* parent, const char* nome) {
  const Tema& t = tema::corrente();
  lv_obj_t* r = ui::contenitore(parent, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(r, LV_PCT(100));
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  ui::etichetta(r, nome, &font_16, t.text2);
  return ui::etichetta(r, "--", &font_16, t.text);
}

lv_obj_t* casella(lv_obj_t* parent, const char* nome) {
  const Tema& t = tema::corrente();
  lv_obj_t* c = ui::contenitore(parent, LV_FLEX_FLOW_COLUMN, 2);
  lv_obj_set_style_bg_color(c, t.surface2, 0);
  lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(c, 8, 0);
  lv_obj_set_style_pad_hor(c, 12, 0);
  lv_obj_set_style_pad_ver(c, 8, 0);
  lv_obj_set_size(c, 133, 64);
  lv_obj_t* v = ui::etichetta(c, "--", &font_24, t.text);
  ui::etichetta(c, nome, &font_12, t.text2);
  return v;
}

lv_obj_t* colonna(lv_obj_t* parent, const char* titolo) {
  lv_obj_t* s = ui::scheda(parent);
  lv_obj_set_size(s, 318, LV_PCT(100));
  ui::titoletto(s, titolo);
  return s;
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 14, 0);

  lv_obj_t* sopra = ui::contenitore(c, LV_FLEX_FLOW_ROW, 14);
  lv_obj_set_size(sopra, LV_PCT(100), 222);
  lv_obj_t* sotto = ui::contenitore(c, LV_FLEX_FLOW_ROW, 14);
  lv_obj_set_width(sotto, LV_PCT(100));
  lv_obj_set_flex_grow(sotto, 1);

  // Collegamento
  lv_obj_t* col = colonna(sopra, "COLLEGAMENTO");
  lv_obj_t* stato = ui::contenitore(col, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_flex_align(stato, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  puntoColl = lv_obj_create(stato);
  lv_obj_remove_style_all(puntoColl);
  lv_obj_set_size(puntoColl, 14, 14);
  lv_obj_set_style_radius(puntoColl, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(puntoColl, LV_OPA_COVER, 0);
  testoColl = ui::etichetta(stato, "--", &font_24, t.text);
  valSegnale = riga(col, "Segnale");
  barraSegnale = lv_bar_create(col);
  lv_obj_set_size(barraSegnale, LV_PCT(100), 10);
  lv_bar_set_range(barraSegnale, -95, -35);
  lv_obj_set_style_bg_color(barraSegnale, t.surface2, 0);
  lv_obj_set_style_bg_color(barraSegnale, t.ok, LV_PART_INDICATOR);
  valRipetuti = riga(col, "Comandi ripetuti");
  valUltima = riga(col, "Ultima risposta");

  // Motore
  lv_obj_t* mot = colonna(sopra, "MOTORE · PASSI");
  displayPos = display7::crea(mot, 34, t.accent, t.segOffAccent);
  display7::testo(displayPos, "___ ___");
  valDriver = riga(mot, "Driver");
  lv_obj_t* fc = ui::contenitore(mot, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(fc, LV_PCT(100));
  chipFcMin = ui::chip(fc, &font_14);
  chipFcMax = ui::chip(fc, &font_14);
  for (lv_obj_t* x : {chipFcMin, chipFcMax}) {
    lv_obj_set_flex_grow(x, 1);
    lv_obj_set_style_text_align(x, LV_TEXT_ALIGN_CENTER, 0);
  }
  valAzzerata = riga(mot, "Ultimo azzeramento");

  // Contatori
  lv_obj_t* con = colonna(sopra, "CONTATORI");
  lv_obj_t* griglia = ui::contenitore(con, LV_FLEX_FLOW_ROW_WRAP, 10);
  lv_obj_set_width(griglia, 276);
  valMovimenti = casella(griglia, "movimenti");
  valAzzeramenti = casella(griglia, "azzeramenti");
  valAcceso = casella(griglia, "pannello acceso");
  valVersione = casella(griglia, "antenna");

  // Ultimi eventi
  lv_obj_t* ev = ui::scheda(sotto);
  lv_obj_set_height(ev, LV_PCT(100));
  lv_obj_set_flex_grow(ev, 2);
  lv_obj_set_style_pad_row(ev, 4, 0);
  lv_obj_add_flag(ev, LV_OBJ_FLAG_SCROLLABLE);
  ui::titoletto(ev, "ULTIMI EVENTI");
  nessunEvento = ui::etichetta(ev, "Nessun evento dall'accensione", &font_16, t.text2);
  for (uint8_t i = 0; i < eventi::MAX_EVENTI; i++) {
    lv_obj_t* r = ui::contenitore(ev, LV_FLEX_FLOW_ROW, 12);
    lv_obj_set_width(r, LV_PCT(100));
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    oreEventi[i] = ui::etichetta(r, "", &font_16, t.text2);
    lv_obj_set_width(oreEventi[i], 64);
    chipEventi[i] = ui::chip(r, &font_12);
    lv_obj_set_width(chipEventi[i], 118);
    lv_obj_set_style_text_align(chipEventi[i], LV_TEXT_ALIGN_CENTER, 0);
    testiEventi[i] = ui::etichetta(r, "", &font_16, t.text);
    righeEventi[i] = r;
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
  }

  // Potenza e ROS (con il wattmetro)
  lv_obj_t* pw = ui::scheda(sotto);
  lv_obj_set_height(pw, LV_PCT(100));
  lv_obj_set_flex_grow(pw, 1);
  lv_obj_set_style_bg_color(pw, t.bg, 0);
  lv_obj_set_style_border_color(pw, t.border, 0);
  lv_obj_set_style_border_width(pw, 2, 0);
  ui::titoletto(pw, "POTENZA E ROS");
  ui::etichetta(pw, "— W    ROS —", &font_28, t.text2);
  lv_obj_t* nota = ui::etichetta(pw, "Disponibile con il wattmetro (2× AD8307 + ADS1115).",
                                 &font_14, t.text2);
  lv_label_set_long_mode(nota, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(nota, LV_PCT(100));
}

void aggiorna(const link_antenna::Stato& s) {
  const Tema& t = tema::corrente();
  char buf[48];

  // Collegamento
  lv_obj_set_style_bg_color(puntoColl, s.collegato ? t.ok : t.danger, 0);
  lv_label_set_text(testoColl, !s.peerImpostato ? "MAC non impostato" : s.collegato ? "Attivo" : "Assente");
  if (s.rssi) {
    lv_label_set_text_fmt(valSegnale, "%d dBm", s.rssi);
    lv_bar_set_value(barraSegnale, s.rssi, LV_ANIM_OFF);
  } else {
    lv_label_set_text(valSegnale, "--");
    lv_bar_set_value(barraSegnale, -95, LV_ANIM_OFF);
  }
  lv_label_set_text_fmt(valRipetuti, "%lu su %lu", (unsigned long)s.ripetuti, (unsigned long)s.inviati);
  if (s.ricevuta) {
    snprintf(buf, sizeof(buf), "%.1f s fa", s.etaMs / 1000.0);
    lv_label_set_text(valUltima, buf);
  } else {
    lv_label_set_text(valUltima, "mai");
  }

  // Motore
  if (s.ricevuta) {
    long p = (long)s.r.posizione;
    if (p < 0) p = 0;
    char cifre[8];
    snprintf(cifre, sizeof(cifre), "%6ld", p % 1000000);
    for (char* q = cifre; *q; q++)
      if (*q == ' ') *q = '_';
    snprintf(buf, sizeof(buf), "%.3s %.3s", cifre, cifre + 3);
    display7::testo(displayPos, buf);
    lv_label_set_text(valDriver, (s.r.flag & proto::FLAG_DRIVER) ? "Abilitato" : "Disabilitato");
    bool fmin = s.r.flag & proto::FLAG_FC_MIN, fmax = s.r.flag & proto::FLAG_FC_MAX;
    lv_label_set_text(chipFcMin, fmin ? "FC MIN · premuto" : "FC MIN · libero");
    lv_label_set_text(chipFcMax, fmax ? "FC MAX · premuto" : "FC MAX · libero");
    ui::coloriChip(chipFcMin, fmin ? t.accentBg : t.surface2, fmin ? t.accent : t.text);
    ui::coloriChip(chipFcMax, fmax ? t.accentBg : t.surface2, fmax ? t.accent : t.text);
    lv_label_set_text(valVersione, (s.r.flag & proto::FLAG_VERSIONE_TX) ? "TX" : "RX");
  } else {
    lv_label_set_text(chipFcMin, "FC MIN · —");
    lv_label_set_text(chipFcMax, "FC MAX · —");
    ui::coloriChip(chipFcMin, t.surface2, t.text2);
    ui::coloriChip(chipFcMax, t.surface2, t.text2);
  }

  // Contatori
  eventi::Contatori c = eventi::contatori();
  lv_label_set_text(valAzzerata, (s.ricevuta && !(s.r.flag & proto::FLAG_AZZERATA)) ? "posizione ignota"
                                                                                  : c.ultimoZero);
  lv_label_set_text_fmt(valMovimenti, "%lu", (unsigned long)c.movimenti);
  lv_label_set_text_fmt(valAzzeramenti, "%lu", (unsigned long)c.azzeramenti);
  uint32_t min = millis() / 60000;
  lv_label_set_text_fmt(valAcceso, "%lu h %02lu'", (unsigned long)(min / 60), (unsigned long)(min % 60));

  // Eventi
  eventi::Evento e[eventi::MAX_EVENTI];
  uint8_t n = eventi::elenco(e, eventi::MAX_EVENTI);
  if (n) lv_obj_add_flag(nessunEvento, LV_OBJ_FLAG_HIDDEN);
  for (uint8_t i = 0; i < eventi::MAX_EVENTI; i++) {
    if (i >= n) {
      lv_obj_add_flag(righeEventi[i], LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    lv_obj_clear_flag(righeEventi[i], LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(oreEventi[i], e[i].ora);
    lv_label_set_text(testiEventi[i], e[i].testo);
    switch (e[i].tipo) {
      case eventi::Tipo::ACCORDATA:
        lv_label_set_text(chipEventi[i], "ACCORDATA");
        ui::coloriChip(chipEventi[i], t.okBg, t.okText);
        break;
      case eventi::Tipo::ZERO:
        lv_label_set_text(chipEventi[i], "ZERO");
        ui::coloriChip(chipEventi[i], t.infoBg, t.infoText);
        break;
      case eventi::Tipo::ERRORE:
        lv_label_set_text(chipEventi[i], "ERRORE");
        ui::coloriChip(chipEventi[i], t.dangerBg, t.dangerText);
        break;
      case eventi::Tipo::COLLEGAMENTO:
        lv_label_set_text(chipEventi[i], "COLLEGAMENTO");
        ui::coloriChip(chipEventi[i], t.surface2, t.text);
        break;
    }
  }
}

}  // namespace pag_diagnostica
