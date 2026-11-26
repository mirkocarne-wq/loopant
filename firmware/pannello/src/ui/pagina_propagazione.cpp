// Schermata Propagazione (requisiti P1-P3, docs/software/09-propagazione.md).
#include <stdio.h>
#include <time.h>

#include <initializer_list>

#include "../impostazioni.h"
#include "../indici.h"
#include "../locator.h"
#include "../mappa.h"
#include "../sole.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"

namespace pag_propagazione {
namespace {

lv_obj_t* mappaObj;
lv_obj_t* barraGiorno;
lv_obj_t* fasciaGiorno;
lv_obj_t* lineaOra;
lv_obj_t* testoOra;
lv_obj_t* testoAlba;
lv_obj_t* testoTramonto;
lv_obj_t* chipGreyLine;
lv_obj_t* valori[4];
lv_obj_t* notaIndici;
lv_obj_t* cond[indici::FASCE * 2];  // per fascia: giorno, notte

time_t ultimoDisegno = 0;
const lv_coord_t LARGHEZZA_BARRA = 568;

bool oraValida(time_t t) { return t > 1700000000; }

void hhmm(double ore, char* out, size_t n) {
  int m = (int)(ore * 60 + 0.5) % (24 * 60);
  snprintf(out, n, "%02d:%02d", m / 60, m % 60);
}

void coloriCondizione(lv_obj_t* c, indici::Condizione k) {
  const Tema& t = tema::corrente();
  lv_label_set_text(c, indici::nome(k));
  switch (k) {
    case indici::Condizione::BUONA: ui::coloriChip(c, t.okBg, t.okText); break;
    case indici::Condizione::DISCRETA: ui::coloriChip(c, t.accentBg, t.accent); break;
    case indici::Condizione::SCARSA: ui::coloriChip(c, t.dangerBg, t.dangerText); break;
    default: ui::coloriChip(c, t.surface2, t.text2); break;
  }
}

lv_obj_t* tessera(lv_obj_t* parent, const char* nome) {
  const Tema& t = tema::corrente();
  lv_obj_t* c = ui::contenitore(parent, LV_FLEX_FLOW_COLUMN, 0);
  lv_obj_set_flex_grow(c, 1);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_color(c, t.surface2, 0);
  lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(c, 8, 0);
  lv_obj_set_style_pad_ver(c, 4, 0);
  lv_obj_t* v = ui::etichetta(c, "—", &font_22, t.text);
  ui::etichetta(c, nome, &font_12, t.text2);
  return v;
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  lv_obj_set_style_pad_column(c, 16, 0);

  // Sinistra: mappa e fascia giorno/notte
  lv_obj_t* sx = ui::contenitore(c, LV_FLEX_FLOW_COLUMN, 12);
  lv_obj_set_size(sx, 600, LV_PCT(100));
  mappaObj = mappa::crea(sx, 600, true);
  lv_obj_set_style_radius(mappaObj, 12, 0);
  lv_obj_set_style_clip_corner(mappaObj, true, 0);

  lv_obj_t* fascia = ui::scheda(sx);
  lv_obj_set_width(fascia, LV_PCT(100));
  lv_obj_set_flex_grow(fascia, 1);
  lv_obj_set_style_pad_row(fascia, 6, 0);
  lv_obj_t* r = ui::contenitore(fascia, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(r, LV_PCT(100));
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  ui::titoletto(r, "GIORNO E NOTTE ALLA STAZIONE · UTC");
  testoOra = ui::etichetta(r, "", &font_14, t.accent);
  barraGiorno = lv_obj_create(fascia);
  lv_obj_remove_style_all(barraGiorno);
  lv_obj_set_size(barraGiorno, LARGHEZZA_BARRA, 22);
  lv_obj_set_style_bg_color(barraGiorno, t.surface2, 0);
  lv_obj_set_style_bg_opa(barraGiorno, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(barraGiorno, 6, 0);
  lv_obj_set_style_clip_corner(barraGiorno, true, 0);
  fasciaGiorno = lv_obj_create(barraGiorno);
  lv_obj_remove_style_all(fasciaGiorno);
  lv_obj_set_style_bg_color(fasciaGiorno, t.accentBg, 0);
  lv_obj_set_style_bg_opa(fasciaGiorno, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(fasciaGiorno, t.accent, 0);
  lv_obj_set_style_border_width(fasciaGiorno, 3, 0);
  lv_obj_set_style_border_side(fasciaGiorno, (lv_border_side_t)(LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT), 0);
  lv_obj_set_height(fasciaGiorno, 22);
  lineaOra = lv_obj_create(barraGiorno);
  lv_obj_remove_style_all(lineaOra);
  lv_obj_set_size(lineaOra, 3, 22);
  lv_obj_set_style_bg_color(lineaOra, t.text, 0);
  lv_obj_set_style_bg_opa(lineaOra, LV_OPA_COVER, 0);
  lv_obj_t* ore = ui::contenitore(fascia, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(ore, LARGHEZZA_BARRA);
  lv_obj_set_flex_align(ore, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  for (const char* h : {"00", "06", "12", "18", "24"}) ui::etichetta(ore, h, &font_12, t.text2);

  // Destra (compatta: quattro fasce di bande nell'ultima scheda)
  lv_obj_t* dx = ui::contenitore(c, LV_FLEX_FLOW_COLUMN, 8);
  lv_obj_set_height(dx, LV_PCT(100));
  lv_obj_set_flex_grow(dx, 1);

  lv_obj_t* st = ui::scheda(dx);
  lv_obj_set_width(st, LV_PCT(100));
  lv_obj_set_style_pad_ver(st, 10, 0);
  ui::titoletto(st, "ALLA STAZIONE");
  lv_obj_t* at = ui::contenitore(st, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(at, LV_PCT(100));
  lv_obj_t* a = ui::contenitore(at, LV_FLEX_FLOW_COLUMN, 2);
  lv_obj_set_flex_grow(a, 1);
  ui::etichetta(a, "Alba (UTC)", &font_14, t.text2);
  testoAlba = ui::etichetta(a, "—", &font_26, t.text);
  lv_obj_t* b = ui::contenitore(at, LV_FLEX_FLOW_COLUMN, 2);
  lv_obj_set_flex_grow(b, 1);
  ui::etichetta(b, "Tramonto (UTC)", &font_14, t.text2);
  testoTramonto = ui::etichetta(b, "—", &font_26, t.text);
  chipGreyLine = ui::chip(st, &font_14);
  lv_obj_set_width(chipGreyLine, LV_PCT(100));
  lv_label_set_long_mode(chipGreyLine, LV_LABEL_LONG_WRAP);

  lv_obj_t* in = ui::scheda(dx);
  lv_obj_set_width(in, LV_PCT(100));
  lv_obj_set_style_pad_ver(in, 10, 0);
  ui::titoletto(in, "INDICI SOLARI");
  lv_obj_t* tes = ui::contenitore(in, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(tes, LV_PCT(100));
  const char* nomi[] = {"SFI", "SSN", "A", "K"};
  for (int i = 0; i < 4; i++) valori[i] = tessera(tes, nomi[i]);
  notaIndici = ui::etichetta(in, "", &font_12, t.text2);
  lv_label_set_long_mode(notaIndici, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(notaIndici, LV_PCT(100));

  lv_obj_t* bd = ui::scheda(dx);
  lv_obj_set_width(bd, LV_PCT(100));
  lv_obj_set_flex_grow(bd, 1);
  lv_obj_set_style_pad_ver(bd, 10, 0);
  lv_obj_set_style_pad_row(bd, 4, 0);
  lv_obj_t* intest = ui::contenitore(bd, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(intest, LV_PCT(100));
  const char* col[] = {"BANDE", "GIORNO", "NOTTE"};
  for (int i = 0; i < 3; i++) {
    lv_obj_t* l = ui::titoletto(intest, col[i]);
    if (i) lv_obj_set_flex_grow(l, 1);
    else lv_obj_set_width(l, 84);
  }
  for (int i = 0; i < indici::FASCE; i++) {
    lv_obj_t* rr = ui::contenitore(bd, LV_FLEX_FLOW_ROW, 8);
    lv_obj_set_width(rr, LV_PCT(100));
    lv_obj_set_flex_align(rr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* n = ui::etichetta(rr, indici::FASCIA_NOME[i], &font_16, t.text);
    lv_obj_set_width(n, 84);
    for (int j = 0; j < 2; j++) {
      lv_obj_t* ch = ui::chip(rr, &font_14);
      lv_obj_set_style_pad_ver(ch, 3, 0);
      lv_obj_set_flex_grow(ch, 1);
      lv_obj_set_style_text_align(ch, LV_TEXT_ALIGN_CENTER, 0);
      cond[i * 2 + j] = ch;
    }
  }
  ultimoDisegno = 0;
}

void aggiorna(const link_antenna::Stato&) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  time_t adesso = time(nullptr);
  double lat = 0, lon = 0;
  bool staz = locator::aCoordinate(imp.locator, lat, lon);

  if (!oraValida(adesso)) {
    lv_label_set_text(testoOra, "ora non impostata");
    lv_label_set_text(chipGreyLine, "Imposta l'ora (WiFi o console) per la grey line");
    ui::coloriChip(chipGreyLine, t.surface2, t.text2);
  } else {
    if (adesso - ultimoDisegno >= 60) {  // la notte si sposta di 0,25 gradi al minuto
      ultimoDisegno = adesso;
      mappa::aggiorna(mappaObj, adesso, staz, lat, lon, imp.nominativo[0] ? imp.nominativo : "Stazione");
    }
    double h = sole::oreUtc(adesso);
    char buf[48];
    hhmm(h, buf, sizeof(buf));
    lv_label_set_text_fmt(testoOra, "ora %s", buf);
    lv_obj_set_x(lineaOra, (lv_coord_t)(h / 24.0 * LARGHEZZA_BARRA) - 1);

    if (!staz) {
      lv_label_set_text(chipGreyLine, "Imposta il locator in Impostazioni");
      ui::coloriChip(chipGreyLine, t.surface2, t.text2);
      lv_obj_add_flag(fasciaGiorno, LV_OBJ_FLAG_HIDDEN);
    } else {
      sole::AlbaTramonto at = sole::albaTramonto(adesso, lat, lon);
      if (at.sempreGiorno || at.sempreNotte) {
        lv_label_set_text(testoAlba, "—");
        lv_label_set_text(testoTramonto, "—");
        lv_label_set_text(chipGreyLine, at.sempreGiorno ? "Sole sempre sopra l'orizzonte" : "Notte polare");
        lv_obj_add_flag(fasciaGiorno, LV_OBJ_FLAG_HIDDEN);
      } else {
        hhmm(at.alba, buf, sizeof(buf));
        lv_label_set_text(testoAlba, buf);
        hhmm(at.tramonto, buf, sizeof(buf));
        lv_label_set_text(testoTramonto, buf);
        double prossimo = h < at.alba ? at.alba : (h < at.tramonto ? at.tramonto : at.alba + 24);
        int min = (int)((prossimo - h) * 60 + 0.5);
        if (min >= 60) snprintf(buf, sizeof(buf), "Grey line sulla stazione tra %d h %d min", min / 60, min % 60);
        else snprintf(buf, sizeof(buf), "Grey line sulla stazione tra %d min", min);
        lv_label_set_text(chipGreyLine, buf);
        // fascia del giorno (anche a cavallo della mezzanotte UTC)
        lv_obj_clear_flag(fasciaGiorno, LV_OBJ_FLAG_HIDDEN);
        double inizio = at.alba, durata = at.tramonto - at.alba;
        if (durata < 0) durata += 24;
        lv_obj_set_x(fasciaGiorno, (lv_coord_t)(inizio / 24.0 * LARGHEZZA_BARRA));
        lv_obj_set_width(fasciaGiorno, (lv_coord_t)(durata / 24.0 * LARGHEZZA_BARRA));
      }
      ui::coloriChip(chipGreyLine, t.accentBg, t.accent);
    }
  }

  indici::Dati d = indici::dati();
  if (d.validi) {
    int v[4] = {d.sfi, d.ssn, d.a, d.k};
    for (int i = 0; i < 4; i++) lv_label_set_text_fmt(valori[i], "%d", v[i]);
    char quando[8] = "—";
    struct tm u;
    gmtime_r(&d.aggiornato, &u);
    snprintf(quando, sizeof(quando), "%02d:%02d", u.tm_hour, u.tm_min);
    lv_label_set_text_fmt(notaIndici, "Aggiornati alle %s UTC (hamqsl.com)%s%s", quando, d.errore[0] ? " · " : "", d.errore);
  } else {
    for (int i = 0; i < 4; i++) lv_label_set_text(valori[i], "—");
    lv_label_set_text(notaIndici, d.errore[0] ? d.errore : "Servono il WiFi e la rete (Impostazioni)");
  }
  for (int i = 0; i < indici::FASCE; i++) {
    coloriCondizione(cond[i * 2], d.giorno[i]);
    coloriCondizione(cond[i * 2 + 1], d.notte[i]);
  }
}

void mostrata() { ultimoDisegno = 0; }

}  // namespace pag_propagazione
