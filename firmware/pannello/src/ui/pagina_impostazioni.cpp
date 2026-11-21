// Schermata Impostazioni (requisiti I1-I5): stazione, tema, antenna, screensaver, schermo e ora, WiFi.
#include <collegamento.h>
#include <ctype.h>
#include <string.h>

#include "../impostazioni.h"
#include "../ora.h"
#include "../rete.h"
#include "../schermo.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"
#include "screensaver.h"
#include "ui.h"

namespace pag_impostazioni {
namespace {

lv_obj_t* tastiera = nullptr;
lv_obj_t* campoNominativo;
lv_obj_t* campoLocator;
lv_obj_t* campoSsid;
lv_obj_t* campoPassword;
lv_obj_t* testoRete;
lv_obj_t* testoLuce;

// --- Gruppi di pulsanti a scelta singola -----------------------------------------------------

struct Gruppo {
  uint8_t* valore;
  const uint8_t* valori;
  uint8_t n;
  lv_obj_t* pulsanti[4];
  void (*cambiato)();
};
Gruppo gruppi[6];
uint8_t numGruppi = 0;

void evidenzia(Gruppo& g) {
  for (uint8_t i = 0; i < g.n; i++)
    ui::stilePulsante(g.pulsanti[i], *g.valore == g.valori[i] ? ui::Stile::PRIMARIO : ui::Stile::NORMALE);
}

void premiGruppo(lv_event_t* e) {
  uintptr_t codice = (uintptr_t)lv_event_get_user_data(e);
  Gruppo& g = gruppi[codice >> 4];
  *g.valore = g.valori[codice & 0x0F];
  impostazioni::salva();
  evidenzia(g);
  if (g.cambiato) g.cambiato();
}

void gruppo(lv_obj_t* parent, const char* const* voci, const uint8_t* valori, uint8_t n, uint8_t* valore,
            void (*cambiato)() = nullptr) {
  uint8_t id = numGruppi++;
  Gruppo& g = gruppi[id];
  g.valore = valore;
  g.valori = valori;
  g.n = n;
  g.cambiato = cambiato;
  lv_obj_t* r = ui::contenitore(parent, LV_FLEX_FLOW_ROW, 6);
  lv_obj_set_width(r, LV_PCT(100));
  for (uint8_t i = 0; i < n; i++) {
    lv_obj_t* b = ui::pulsante(r, voci[i], &font_16, ui::Stile::NORMALE);
    lv_obj_set_height(b, 46);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_add_event_cb(b, premiGruppo, LV_EVENT_CLICKED, (void*)(uintptr_t)((id << 4) | i));
    g.pulsanti[i] = b;
  }
  evidenzia(g);
}

// --- Campi di testo e tastiera --------------------------------------------------------------

void copia(char* dst, size_t n, const char* src, bool maiuscolo) {
  size_t i = 0;
  for (; src[i] && i < n - 1; i++) dst[i] = maiuscolo ? (char)toupper((unsigned char)src[i]) : src[i];
  dst[i] = 0;
}

void conferma(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* ta = lv_keyboard_get_textarea(tastiera);
  if (code == LV_EVENT_READY && ta) {
    Impostazioni& imp = impostazioni::get();
    const char* v = lv_textarea_get_text(ta);
    if (ta == campoNominativo) {
      copia(imp.nominativo, sizeof(imp.nominativo), v, true);
      ui::richiediRicostruzione();  // il nominativo compare nella barra in alto
    } else if (ta == campoLocator) {
      copia(imp.locator, sizeof(imp.locator), v, false);
      if (strlen(imp.locator) >= 2) {
        imp.locator[0] = (char)toupper((unsigned char)imp.locator[0]);
        imp.locator[1] = (char)toupper((unsigned char)imp.locator[1]);
      }
      if (strlen(imp.locator) == 6) {
        imp.locator[4] = (char)tolower((unsigned char)imp.locator[4]);
        imp.locator[5] = (char)tolower((unsigned char)imp.locator[5]);
      }
      ui::richiediRicostruzione();
    } else if (ta == campoSsid) {
      copia(imp.wifiSsid, sizeof(imp.wifiSsid), v, false);
    } else if (ta == campoPassword) {
      copia(imp.wifiPassword, sizeof(imp.wifiPassword), v, false);
    }
    impostazioni::salva();
    if (ta == campoSsid || ta == campoPassword) rete::riavvia();
  }
  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
    if (ta) lv_obj_clear_state(ta, LV_STATE_FOCUSED);
  }
}

void apriTastiera(lv_event_t* e) {
  lv_obj_t* ta = lv_event_get_target(e);
  lv_keyboard_set_textarea(tastiera, ta);
  bool maiuscole = ta == campoNominativo || ta == campoLocator;
  lv_keyboard_set_mode(tastiera, maiuscole ? LV_KEYBOARD_MODE_TEXT_UPPER : LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_obj_clear_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t* campo(lv_obj_t* parent, const char* nome, const char* valore, uint32_t max, lv_coord_t larghezza,
                bool password = false) {
  const Tema& t = tema::corrente();
  lv_obj_t* box = ui::contenitore(parent, LV_FLEX_FLOW_COLUMN, 4);
  if (larghezza < 0) {
    lv_obj_set_flex_grow(box, 1);
    larghezza = LV_PCT(100);
  }
  ui::etichetta(box, nome, &font_14, t.text2);
  lv_obj_t* ta = lv_textarea_create(box);
  lv_textarea_set_one_line(ta, true);
  lv_textarea_set_max_length(ta, max);
  lv_textarea_set_password_mode(ta, password);
  lv_textarea_set_text(ta, valore);
  lv_obj_set_size(ta, larghezza, 48);
  lv_obj_set_style_bg_color(ta, t.bg, 0);
  lv_obj_set_style_border_color(ta, t.borderStrong, 0);
  lv_obj_set_style_border_color(ta, t.accent, LV_STATE_FOCUSED);
  lv_obj_set_style_border_width(ta, 2, 0);
  lv_obj_set_style_radius(ta, 10, 0);
  lv_obj_set_style_text_color(ta, t.text, 0);
  lv_obj_set_style_text_font(ta, &font_20, 0);
  lv_obj_add_event_cb(ta, apriTastiera, LV_EVENT_CLICKED, nullptr);
  return ta;
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

// --- Azioni ---------------------------------------------------------------------------------

void scegliTema(lv_event_t* e) {
  impostazioni::get().tema = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  impostazioni::salva();
  ui::richiediRicostruzione();
}

void apriCalibrazione(lv_event_t*) { ui::vaiA(ui::CALIBRAZIONE); }
void anteprima(lv_event_t*) { screensaver::anteprima(); }
void fusoCambiato() { ora::applicaFuso(); }
void aggiornaIndici(lv_event_t*) { rete::aggiornaIndiciOra(); }

void luce(lv_event_t* e) {
  lv_obj_t* s = lv_event_get_target(e);
  uint8_t v = (uint8_t)lv_slider_get_value(s);
  lv_label_set_text_fmt(testoLuce, "%u %%", v);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    impostazioni::get().luminosita = v;
    impostazioni::salva();
  }
  schermo::luminosita(v);
}

lv_obj_t* sezione(lv_obj_t* parent, const char* titolo) {
  lv_obj_t* s = ui::scheda(parent);
  lv_obj_set_width(s, LV_PCT(100));
  lv_obj_set_style_pad_ver(s, 12, 0);
  ui::titoletto(s, titolo);
  return s;
}

lv_obj_t* colonna(lv_obj_t* parent) {
  lv_obj_t* c = ui::contenitore(parent, LV_FLEX_FLOW_COLUMN, 12);
  lv_obj_set_height(c, LV_PCT(100));
  lv_obj_set_flex_grow(c, 1);
  lv_obj_add_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(c, LV_DIR_VER);
  return c;
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  Impostazioni& imp = impostazioni::get();
  lv_obj_set_style_pad_column(c, 16, 0);
  numGruppi = 0;

  lv_obj_t* sx = colonna(c);
  lv_obj_t* dx = colonna(c);

  // Stazione
  lv_obj_t* st = sezione(sx, "STAZIONE");
  lv_obj_t* riga = ui::contenitore(st, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(riga, LV_PCT(100));
  campoNominativo = campo(riga, "Nominativo", imp.nominativo, sizeof(imp.nominativo) - 1, -1);
  campoLocator = campo(riga, "Locator", imp.locator, sizeof(imp.locator) - 1, 150);

  // Tema
  lv_obj_t* te = sezione(sx, "TEMA");
  lv_obj_t* griglia = ui::contenitore(te, LV_FLEX_FLOW_ROW_WRAP, 10);
  lv_obj_set_width(griglia, LV_PCT(100));
  for (uint8_t i = 0; i < tema::NUMERO; i++) {
    const Tema& x = tema::di(i);
    bool sel = i == tema::indiceCorrente();
    lv_obj_t* b = lv_btn_create(griglia);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, 210, 70);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(b, x.bg, 0);
    lv_obj_set_style_border_color(b, sel ? t.accent : t.border, 0);
    lv_obj_set_style_border_width(b, sel ? 3 : 1, 0);
    lv_obj_t* f = ui::etichetta(b, "7 074", &font_24, x.accent);
    lv_obj_align(f, LV_ALIGN_TOP_LEFT, 12, 6);
    lv_obj_t* n = ui::etichetta(b, x.nome, &font_16, x.text);
    lv_obj_align(n, LV_ALIGN_BOTTOM_LEFT, 12, -8);
    lv_obj_add_event_cb(b, scegliTema, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }

  // Antenna
  lv_obj_t* an = sezione(sx, "ANTENNA");
  char mac[18];
  Collegamento::scriviMac(imp.peer, mac);
  lv_obj_t* l = ui::etichetta(an, "", &font_16, t.text);
  lv_label_set_text_fmt(l, "MAC dell'antenna: %s", mac);
  uint8_t mio[6];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, mac);
  l = ui::etichetta(an, "", &font_14, t.text2);
  lv_label_set_text_fmt(l, "MAC di questo pannello: %s\nIl MAC dell'antenna si imposta dalla console: mac AA:BB:…", mac);
  lv_obj_t* cal = ui::pulsante(an, "Calibrazione antenna  " LV_SYMBOL_RIGHT, &font_18, ui::Stile::NORMALE);
  lv_obj_set_size(cal, LV_PCT(100), 52);
  lv_obj_add_event_cb(cal, apriCalibrazione, LV_EVENT_CLICKED, nullptr);

  // Screensaver
  static const char* const STILI[] = {"Orologio", "Frequenza", "Grey line", "Spento"};
  static const uint8_t V_STILI[] = {0, 1, 2, 3};
  static const char* const ATTESE[] = {"Mai", "5 min", "15 min", "30 min"};
  static const uint8_t V_ATTESE[] = {0, 5, 15, 30};
  lv_obj_t* ss = sezione(dx, "SCREENSAVER");
  ui::etichetta(ss, "Stile", &font_14, t.text2);
  gruppo(ss, STILI, V_STILI, 4, &imp.screensaver);
  ui::etichetta(ss, "Parte dopo", &font_14, t.text2);
  gruppo(ss, ATTESE, V_ATTESE, 4, &imp.attesa_min);
  lv_obj_t* ap = ui::pulsante(ss, "Anteprima", &font_16, ui::Stile::SCHEDA);
  lv_obj_set_size(ap, 160, 44);
  lv_obj_add_event_cb(ap, anteprima, LV_EVENT_CLICKED, nullptr);

  // Schermo e ora
  static const char* const MODI[] = {"UTC", "Locale", "Entrambe"};
  static const uint8_t V_MODI[] = {0, 1, 2};
  static const char* const FUSI[] = {"UTC", "Europa centrale"};
  static const uint8_t V_FUSI[] = {0, 1};
  lv_obj_t* so = sezione(dx, "SCHERMO E ORA");
  lv_obj_t* lr = ui::contenitore(so, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(lr, LV_PCT(100));
  lv_obj_set_flex_align(lr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  ui::etichetta(lr, "Luminosità (verso da verificare)", &font_14, t.text2);
  testoLuce = ui::etichetta(lr, "", &font_16, t.text);
  lv_label_set_text_fmt(testoLuce, "%u %%", imp.luminosita);
  lv_obj_t* sl = lv_slider_create(so);
  lv_slider_set_range(sl, 10, 100);
  lv_slider_set_value(sl, imp.luminosita, LV_ANIM_OFF);
  lv_obj_set_width(sl, LV_PCT(96));
  lv_obj_set_style_bg_color(sl, t.surface2, 0);
  lv_obj_set_style_bg_color(sl, t.accent, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sl, t.accent, LV_PART_KNOB);
  lv_obj_add_event_cb(sl, luce, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(sl, luce, LV_EVENT_RELEASED, nullptr);
  ui::etichetta(so, "Ora nella barra", &font_14, t.text2);
  gruppo(so, MODI, V_MODI, 3, &imp.modoOra);
  ui::etichetta(so, "Fuso orario dell'ora locale", &font_14, t.text2);
  gruppo(so, FUSI, V_FUSI, 2, &imp.fuso, fusoCambiato);

  // Rete
  lv_obj_t* wf = sezione(dx, "WIFI DI CASA (ORA E INDICI SOLARI)");
  lv_obj_t* rw = ui::contenitore(wf, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(rw, LV_PCT(100));
  campoSsid = campo(rw, "Rete (SSID)", imp.wifiSsid, sizeof(imp.wifiSsid) - 1, -1);
  campoPassword = campo(rw, "Password", imp.wifiPassword, sizeof(imp.wifiPassword) - 1, -1, true);
  testoRete = ui::etichetta(wf, "", &font_14, t.text2);
  lv_label_set_long_mode(testoRete, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(testoRete, LV_PCT(100));
  lv_obj_t* nota = ui::etichetta(wf, "Con il WiFi attivo l'antenna deve usare lo stesso canale del router.",
                                 &font_12, t.text2);
  lv_label_set_long_mode(nota, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(nota, LV_PCT(100));
  lv_obj_t* ai = ui::pulsante(wf, "Aggiorna indici solari", &font_16, ui::Stile::SCHEDA);
  lv_obj_set_size(ai, 240, 44);
  lv_obj_add_event_cb(ai, aggiornaIndici, LV_EVENT_CLICKED, nullptr);

  creaTastiera();
}

void aggiorna(const link_antenna::Stato&) {
  char buf[96];
  rete::stato(buf, sizeof(buf));
  lv_label_set_text(testoRete, buf);
}

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
