// Schermata Memorie (requisiti M1-M4). Tocco: accorda; pressione lunga su una memoria personale: elimina.
#include <bande.h>
#include <stdio.h>
#include <string.h>

#include <initializer_list>

#include "../display7.h"
#include "../impostazioni.h"
#include "../memorie.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"
#include "ui.h"

namespace pag_memorie {
namespace {

enum Filtro : uint8_t { TUTTE, B40, B20, MIE };
const char* NOMI_FILTRI[] = {"Tutte", "40 m", "20 m", "Mie"};

Filtro filtro = TUTTE;
bool collegato = false;

lv_obj_t* griglia;
lv_obj_t* btnFiltri[4];
lv_obj_t* finestra = nullptr;  // finestra "nuova memoria" / "elimina"
lv_obj_t* tastiera = nullptr;
lv_obj_t* campoNome;
uint32_t freqNuova = 0;
uint8_t daEliminare = 0;

// codice memoria: bit 7 = personale, resto = indice
constexpr uint8_t PERSONALE = 0x80;

void ricostruisci();

void testoDisplay(uint32_t hz, char* out, size_t n) {
  uint32_t decimi = (hz + 50) / 100, khz = decimi / 10;
  char p[6];
  snprintf(p, sizeof(p), "%5lu", (unsigned long)(khz % 100000));
  for (char* q = p; *q; q++)
    if (*q == ' ') *q = '_';
  snprintf(out, n, "%c%c %c%c%c,%lu", p[0], p[1], p[2], p[3], p[4], (unsigned long)(decimi % 10));
}

void coloriModo(memorie::Modo m, lv_color_t& fondo, lv_color_t& testo) {
  const Tema& t = tema::corrente();
  switch (m) {
    case memorie::Modo::FT8:
    case memorie::Modo::FT4:
    case memorie::Modo::WSPR: fondo = t.infoBg; testo = t.infoText; break;
    case memorie::Modo::CW: fondo = t.surface2; testo = t.text; break;
    case memorie::Modo::SSB: fondo = t.okBg; testo = t.okText; break;
    case memorie::Modo::MIA: fondo = t.accentBg; testo = t.accent; break;
  }
}

const memorie::Memoria& memoria(uint8_t codice) {
  return (codice & PERSONALE) ? memorie::personale(codice & 0x7F) : memorie::predefinita(codice);
}

bool visibile(const memorie::Memoria& m, bool personale) {
  bande::Banda b = bande::di(m.freq_hz);
  switch (filtro) {
    case TUTTE: return true;
    case B40: return b == bande::Banda::B40M;
    case B20: return b == bande::Banda::B20M;
    case MIE: return personale;
  }
  return true;
}

void chiudiFinestra() {
  if (tastiera) {
    lv_obj_del(tastiera);
    tastiera = nullptr;
  }
  if (finestra) {
    lv_obj_del(finestra);
    finestra = nullptr;
  }
}

lv_obj_t* apriFinestra(const char* titolo, lv_coord_t altezza) {
  const Tema& t = tema::corrente();
  chiudiFinestra();
  finestra = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(finestra);
  lv_obj_set_size(finestra, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(finestra, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(finestra, LV_OPA_50, 0);
  lv_obj_add_flag(finestra, LV_OBJ_FLAG_CLICKABLE);  // blocca i tocchi sotto
  lv_obj_t* box = ui::scheda(finestra);
  lv_obj_set_size(box, 560, altezza);
  lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_pad_row(box, 14, 0);
  ui::etichetta(box, titolo, &font_24, t.text);
  return box;
}

// --- Accorda --------------------------------------------------------------------------------

void premiMemoria(lv_event_t* e) {
  uint8_t codice = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  const memorie::Memoria& m = memoria(codice);
  pag_sintonia::proponi(m.freq_hz);
  if (collegato) {
    link_antenna::invia(proto::Tipo::GOTO, m.freq_hz);
    Impostazioni& imp = impostazioni::get();
    if (bande::di(m.freq_hz) == bande::Banda::B40M) imp.ultima40_hz = m.freq_hz;
    else imp.ultima20_hz = m.freq_hz;
    impostazioni::salva();
  }
  ui::vaiA(ui::SINTONIA);
}

// --- Elimina --------------------------------------------------------------------------------

void confermaElimina(lv_event_t*) {
  memorie::elimina(daEliminare);
  chiudiFinestra();
  ricostruisci();
}

void annulla(lv_event_t*) { chiudiFinestra(); }

void tieniMemoria(lv_event_t* e) {
  uint8_t codice = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  if (!(codice & PERSONALE)) return;
  daEliminare = codice & 0x7F;
  lv_obj_t* box = apriFinestra("Eliminare la memoria?", 230);
  ui::etichetta(box, memorie::personale(daEliminare).nome, &font_20, tema::corrente().text2);
  lv_obj_t* r = ui::contenitore(box, LV_FLEX_FLOW_ROW, 12);
  lv_obj_t* a = ui::pulsante(r, "Annulla", &font_20, ui::Stile::NORMALE);
  lv_obj_t* el = ui::pulsante(r, "Elimina", &font_20, ui::Stile::PERICOLO);
  for (lv_obj_t* b : {a, el}) lv_obj_set_size(b, 200, 56);
  lv_obj_add_event_cb(a, annulla, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(el, confermaElimina, LV_EVENT_CLICKED, nullptr);
}

// --- Nuova memoria --------------------------------------------------------------------------

void salvaNuova(lv_event_t*) {
  memorie::aggiungi(lv_textarea_get_text(campoNome), freqNuova);
  chiudiFinestra();
  filtro = MIE;
  ricostruisci();
}

void tastieraFatto(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_READY) salvaNuova(e);
  else chiudiFinestra();
}

void nuovaMemoria(lv_event_t*) {
  const Tema& t = tema::corrente();
  freqNuova = pag_sintonia::frequenzaDigitata();
  bool valida = bande::di(freqNuova) != bande::Banda::NESSUNA;
  lv_obj_t* box = apriFinestra("Nuova memoria", 270);
  char f[16];
  testoDisplay(freqNuova, f, sizeof(f));
  lv_obj_t* r = ui::contenitore(box, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  display7::crea(r, 36, t.accent, t.segOffAccent);
  display7::testo(lv_obj_get_child(r, 0), f);
  ui::etichetta(r, valida ? "kHz (dalla Sintonia)" : "kHz: fuori banda", &font_16, valida ? t.text2 : t.dangerText);

  campoNome = lv_textarea_create(box);
  lv_textarea_set_one_line(campoNome, true);
  lv_textarea_set_max_length(campoNome, sizeof(memorie::Memoria::nome) - 1);
  lv_textarea_set_placeholder_text(campoNome, "Nome della memoria");
  lv_obj_set_size(campoNome, LV_PCT(100), 52);
  lv_obj_set_style_bg_color(campoNome, t.bg, 0);
  lv_obj_set_style_border_color(campoNome, t.accent, 0);
  lv_obj_set_style_border_width(campoNome, 2, 0);
  lv_obj_set_style_radius(campoNome, 10, 0);
  lv_obj_set_style_text_color(campoNome, t.text, 0);
  lv_obj_set_style_text_font(campoNome, &font_22, 0);

  tastiera = lv_keyboard_create(lv_layer_top());
  lv_obj_set_size(tastiera, LV_PCT(100), 260);
  lv_obj_align(tastiera, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(tastiera, t.surface, 0);
  lv_obj_set_style_bg_color(tastiera, t.surface2, LV_PART_ITEMS);
  lv_obj_set_style_text_color(tastiera, t.text, LV_PART_ITEMS);
  lv_obj_set_style_text_font(tastiera, &font_20, LV_PART_ITEMS);
  lv_keyboard_set_textarea(tastiera, campoNome);
  if (valida) lv_obj_add_event_cb(tastiera, tastieraFatto, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(tastiera, tastieraFatto, LV_EVENT_CANCEL, nullptr);
  if (!valida) lv_obj_add_state(campoNome, LV_STATE_DISABLED);
}

// --- Griglia --------------------------------------------------------------------------------

void scheda(uint8_t codice) {
  const Tema& t = tema::corrente();
  const memorie::Memoria& m = memoria(codice);
  lv_obj_t* c = lv_btn_create(griglia);
  lv_obj_remove_style_all(c);
  lv_obj_set_size(c, 228, 98);
  lv_obj_set_style_bg_color(c, t.surface, 0);
  lv_obj_set_style_bg_color(c, t.surface2, LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(c, t.border, 0);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_radius(c, 12, 0);
  lv_obj_set_style_pad_all(c, 12, 0);

  lv_obj_t* nome = ui::etichetta(c, m.nome, &font_16, t.text);
  lv_label_set_long_mode(nome, LV_LABEL_LONG_DOT);
  lv_obj_set_width(nome, 140);
  lv_obj_align(nome, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_t* tag = ui::chip(c, &font_12);
  lv_label_set_text(tag, memorie::nomeModo(m.modo));
  lv_obj_set_style_pad_hor(tag, 8, 0);
  lv_obj_set_style_pad_ver(tag, 3, 0);
  lv_color_t f, x;
  coloriModo(m.modo, f, x);
  ui::coloriChip(tag, f, x);
  lv_obj_align(tag, LV_ALIGN_TOP_RIGHT, 0, 0);

  char buf[16];
  testoDisplay(m.freq_hz, buf, sizeof(buf));
  lv_obj_t* d = display7::crea(c, 28, t.accent, t.segOffAccent);
  display7::testo(d, buf);
  lv_obj_align(d, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  const bande::Info* b = bande::info(bande::di(m.freq_hz));
  lv_obj_t* lb = ui::etichetta(c, b ? b->nome : "", &font_14, t.text2);
  lv_obj_align(lb, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lv_obj_add_event_cb(c, premiMemoria, LV_EVENT_SHORT_CLICKED, (void*)(uintptr_t)codice);
  lv_obj_add_event_cb(c, tieniMemoria, LV_EVENT_LONG_PRESSED, (void*)(uintptr_t)codice);
}

void ricostruisci() {
  const Tema& t = tema::corrente();
  lv_obj_clean(griglia);
  for (uint8_t i = 0; i < memorie::numeroPredefinite(); i++)
    if (visibile(memorie::predefinita(i), false)) scheda(i);
  for (uint8_t i = 0; i < memorie::numeroPersonali(); i++)
    if (visibile(memorie::personale(i), true)) scheda(PERSONALE | i);

  lv_obj_t* n = lv_btn_create(griglia);
  lv_obj_remove_style_all(n);
  lv_obj_set_size(n, 228, 98);
  lv_obj_set_style_radius(n, 12, 0);
  lv_obj_set_style_border_color(n, t.borderStrong, 0);
  lv_obj_set_style_border_width(n, 2, 0);
  lv_obj_set_style_bg_color(n, t.surface2, LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(n, LV_OPA_COVER, LV_STATE_PRESSED);
  lv_obj_t* l = ui::etichetta(n, LV_SYMBOL_PLUS "  Nuova memoria", &font_18, t.text2);
  lv_obj_center(l);
  lv_obj_add_event_cb(n, nuovaMemoria, LV_EVENT_CLICKED, nullptr);

  for (uint8_t i = 0; i < 4; i++) {
    lv_obj_set_style_bg_color(btnFiltri[i], i == filtro ? t.accentBg : t.surface, 0);
    lv_obj_set_style_border_color(btnFiltri[i], i == filtro ? t.accent : t.border, 0);
    lv_obj_set_style_text_color(btnFiltri[i], i == filtro ? t.accent : t.text, 0);
  }
}

void premiFiltro(lv_event_t* e) {
  filtro = (Filtro)(uintptr_t)lv_event_get_user_data(e);
  ricostruisci();
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  memorie::carica();
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 14, 0);
  lv_obj_set_style_pad_top(c, 14, 0);

  lv_obj_t* testa = ui::contenitore(c, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(testa, LV_PCT(100));
  lv_obj_set_flex_align(testa, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* tit = ui::etichetta(testa, "Memorie", &font_24, t.text);
  lv_obj_set_flex_grow(tit, 1);
  ui::etichetta(testa, "Pressione lunga su una memoria personale per eliminarla", &font_14, t.text2);
  for (uint8_t i = 0; i < 4; i++) {
    lv_obj_t* b = ui::pulsante(testa, NOMI_FILTRI[i], &font_16, ui::Stile::SCHEDA);
    lv_obj_set_size(b, 96, 44);
    lv_obj_set_style_radius(b, 22, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_add_event_cb(b, premiFiltro, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    btnFiltri[i] = b;
  }

  griglia = ui::contenitore(c, LV_FLEX_FLOW_ROW_WRAP, 12);
  lv_obj_set_width(griglia, LV_PCT(100));
  lv_obj_set_flex_grow(griglia, 1);
  lv_obj_add_flag(griglia, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(griglia, LV_DIR_VER);
  chiudiFinestra();
  ricostruisci();
}

void aggiorna(const link_antenna::Stato& s) { collegato = s.collegato; }

}  // namespace pag_memorie
