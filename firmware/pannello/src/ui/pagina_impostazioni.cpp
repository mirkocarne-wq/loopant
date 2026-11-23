// Schermata Impostazioni (requisiti I1-I6): stazione, tema, antenna, screensaver, schermo e ora,
// WiFi con ricerca delle reti.
#include <collegamento.h>
#include <ctype.h>
#include <string.h>

#include <initializer_list>

#include "../impostazioni.h"
#include "../link_manopola.h"
#include "../scheda.h"
#include "../motore_antenna.h"
#include "../ora.h"
#include "../rete.h"
#include "../schermo.h"
#include "../server_web.h"
#include "../servizio_qrz.h"
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
lv_obj_t* boxRx;  // parametri CAN del SERVO42D: solo antenna RX
uint8_t gruppoTipo = 0;
uint32_t cambioTipoAlle = 0;  // millis della richiesta di cambio tipo (0 = nessuna)
lv_obj_t* testoCanId;
lv_obj_t* statoMotore;
lv_obj_t* sezioneAntenna;
lv_obj_t* testoManopola;
lv_obj_t* testoPassi;
lv_obj_t* campoQrzUtente;
lv_obj_t* campoQrzPassword;
lv_obj_t* campoQrzChiave;
lv_obj_t* testoQrz;
uint32_t versioneQrz = 0xFFFFFFFF;

// Finestra "Reti WiFi"
lv_obj_t* finestra = nullptr;
lv_obj_t* boxReti = nullptr;
lv_obj_t* elencoReti = nullptr;  // nullptr quando si mostra la password
lv_obj_t* statoReti = nullptr;
lv_obj_t* campoPwdRete = nullptr;
lv_obj_t* btnOcchio = nullptr;
rete::Rete visibili[rete::MAX_RETI];
uint8_t nVisibili = 0;
uint32_t versioneVista = 0;
bool eraInCorso = false;
bool attesaMovimento = false;
char reteScelta[33];

void collegaScelta(const char* password);
void mostraElenco();

// --- Gruppi di pulsanti a scelta singola -----------------------------------------------------

struct Gruppo {
  uint8_t* valore;
  const uint8_t* valori;
  uint8_t n;
  lv_obj_t* pulsanti[4];
  void (*cambiato)();
};
Gruppo gruppi[10];  // id in 4 bit: al massimo 16
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
  if (ta && ta == campoPwdRete) {  // password della rete scelta nella ricerca
    if (code == LV_EVENT_READY) collegaScelta(lv_textarea_get_text(ta));
    else mostraElenco();
    return;
  }
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
    } else if (ta == campoQrzUtente || ta == campoQrzPassword || ta == campoQrzChiave) {
      ImpostazioniQrz& q = impostazioni::qrz();
      if (ta == campoQrzUtente) copia(q.utente, sizeof(q.utente), v, true);
      else if (ta == campoQrzPassword) copia(q.password, sizeof(q.password), v, false);
      else copia(q.chiave, sizeof(q.chiave), v, true);
      impostazioni::salvaQrz();
      servizio_qrz::credenzialiCambiate();
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
  bool maiuscole = ta == campoNominativo || ta == campoLocator || ta == campoQrzUtente || ta == campoQrzChiave;
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
  // tasti speciali (OK, maiuscole, frecce...) nei colori del tema
  lv_obj_set_style_bg_color(tastiera, t.borderStrong, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(tastiera, t.text, LV_PART_ITEMS | LV_STATE_CHECKED);
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

// --- Tipo di antenna e driver del motore dell'antenna RX -------------------------------------

void mostraRx() {
  if (motore_antenna::scelte().tipo == 0) lv_obj_clear_flag(boxRx, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(boxRx, LV_OBJ_FLAG_HIDDEN);
}

void tipoCambiato() {
  mostraRx();
  cambioTipoAlle = millis();
  motore_antenna::inviaTipo();
}

void kbitCambiato() { motore_antenna::inviaCan(); }

// --- Manopola remota ---------------------------------------------------------------------------

void sorgenteCambiata() { link_manopola::salvaScelte(); }

void premiPassi(lv_event_t* e) {
  static const uint16_t PASSI[] = {1, 2, 5, 10, 20, 50, 100, 200, 500};
  const int ultimo = sizeof(PASSI) / sizeof(PASSI[0]) - 1;
  link_manopola::Scelte& m = link_manopola::scelte();
  int i = 0;
  while (i < ultimo && PASSI[i] < m.passiScatto) i++;
  i += (int)(intptr_t)lv_event_get_user_data(e);
  m.passiScatto = PASSI[i < 0 ? 0 : i > ultimo ? ultimo : i];
  lv_label_set_text_fmt(testoPassi, "%u", m.passiScatto);
  link_manopola::salvaScelte();
}

void premiCanId(lv_event_t* e) {
  motore_antenna::Scelte& m = motore_antenna::scelte();
  int v = m.canId + (int)(intptr_t)lv_event_get_user_data(e);
  m.canId = (uint16_t)(v < 1 ? 1 : v > 2047 ? 2047 : v);
  lv_label_set_text_fmt(testoCanId, "%u", m.canId);
  motore_antenna::inviaCan();
}
void anteprima(lv_event_t*) { screensaver::anteprima(); }
void fusoCambiato() { ora::applicaFuso(); }
void aggiornaIndici(lv_event_t*) { rete::aggiornaIndiciOra(); }
void provaQrz(lv_event_t*) { servizio_qrz::provaAccesso(); }

// --- Ricerca delle reti WiFi ----------------------------------------------------------------

void nascondiTastiera() {
  if (!tastiera) return;
  lv_keyboard_set_textarea(tastiera, nullptr);
  lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
}

void chiudiReti(lv_event_t*) {
  if (!finestra) return;
  nascondiTastiera();
  lv_obj_del(finestra);
  finestra = boxReti = elencoReti = statoReti = campoPwdRete = btnOcchio = nullptr;
}

// La ricerca cambia canale: con l'antenna in movimento si aspetta (heartbeat di 3 s)
void cerca(lv_event_t*) {
  link_antenna::Stato s = link_antenna::stato();
  attesaMovimento = s.collegato && (s.r.stato == proto::Stato::IN_MOVIMENTO || s.r.stato == proto::Stato::AZZERAMENTO);
  if (attesaMovimento) {
    if (statoReti) lv_label_set_text(statoReti, "Antenna in movimento: attendi che sia ferma, poi «Cerca di nuovo».");
    return;
  }
  rete::avviaScansione();
  versioneVista = 0xFFFFFFFF;  // ridisegna subito con "ricerca in corso"
}

void collegaScelta(const char* password) {
  rete::collega(reteScelta, password);
  lv_textarea_set_text(campoSsid, reteScelta);
  lv_textarea_set_text(campoPassword, password);
  chiudiReti(nullptr);  // per ultimo: la password può stare nel campo che viene eliminato
}

void premiCollega(lv_event_t*) {
  if (campoPwdRete) collegaScelta(lv_textarea_get_text(campoPwdRete));
}

void tornaElenco(lv_event_t*) { mostraElenco(); }

void apriTastieraRete(lv_event_t*) {
  lv_keyboard_set_textarea(tastiera, campoPwdRete);
  lv_obj_clear_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(tastiera);
}

void mostraNascondi(lv_event_t*) {
  bool nascosta = !lv_textarea_get_password_mode(campoPwdRete);
  lv_textarea_set_password_mode(campoPwdRete, nascosta);
  lv_label_set_text(lv_obj_get_child(btnOcchio, 0), nascosta ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
}

void mostraPassword() {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  nascondiTastiera();
  lv_obj_clean(boxReti);
  elencoReti = statoReti = nullptr;
  lv_obj_set_size(boxReti, 680, 222);
  lv_obj_align(boxReti, LV_ALIGN_TOP_MID, 0, 12);  // sopra la tastiera

  lv_obj_t* tit = ui::etichetta(boxReti, "", &font_22, t.text);
  lv_label_set_text_fmt(tit, "Password per «%s»", reteScelta);
  lv_label_set_long_mode(tit, LV_LABEL_LONG_DOT);
  lv_obj_set_width(tit, LV_PCT(100));

  lv_obj_t* riga = ui::contenitore(boxReti, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(riga, LV_PCT(100));
  campoPwdRete = lv_textarea_create(riga);
  lv_textarea_set_one_line(campoPwdRete, true);
  lv_textarea_set_password_mode(campoPwdRete, true);
  lv_textarea_set_max_length(campoPwdRete, sizeof(imp.wifiPassword) - 1);
  lv_textarea_set_placeholder_text(campoPwdRete, "Password della rete");
  if (!strcmp(reteScelta, imp.wifiSsid)) lv_textarea_set_text(campoPwdRete, imp.wifiPassword);
  else lv_textarea_set_text(campoPwdRete, "");
  lv_obj_set_height(campoPwdRete, 52);
  lv_obj_set_flex_grow(campoPwdRete, 1);
  lv_obj_set_style_bg_color(campoPwdRete, t.bg, 0);
  lv_obj_set_style_border_color(campoPwdRete, t.accent, 0);
  lv_obj_set_style_border_width(campoPwdRete, 2, 0);
  lv_obj_set_style_radius(campoPwdRete, 10, 0);
  lv_obj_set_style_text_color(campoPwdRete, t.text, 0);
  lv_obj_set_style_text_font(campoPwdRete, &font_22, 0);
  lv_obj_add_event_cb(campoPwdRete, apriTastieraRete, LV_EVENT_CLICKED, nullptr);
  btnOcchio = ui::pulsante(riga, LV_SYMBOL_EYE_OPEN, &font_20, ui::Stile::NORMALE);
  lv_obj_set_size(btnOcchio, 64, 52);
  lv_obj_add_event_cb(btnOcchio, mostraNascondi, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* nota = ui::etichetta(boxReti, "La password viene salvata nel pannello.", &font_14, t.text2);
  (void)nota;

  lv_obj_t* pulsanti = ui::contenitore(boxReti, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(pulsanti, LV_PCT(100));
  lv_obj_t* ind = ui::pulsante(pulsanti, LV_SYMBOL_LEFT "  Indietro", &font_18, ui::Stile::NORMALE);
  lv_obj_t* col = ui::pulsante(pulsanti, LV_SYMBOL_OK "  Collega", &font_18, ui::Stile::PRIMARIO);
  for (lv_obj_t* b : {ind, col}) {
    lv_obj_set_height(b, 54);
    lv_obj_set_flex_grow(b, 1);
  }
  lv_obj_add_event_cb(ind, tornaElenco, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(col, premiCollega, LV_EVENT_CLICKED, nullptr);

  apriTastieraRete(nullptr);
  lv_obj_add_state(campoPwdRete, LV_STATE_FOCUSED);
}

void premiRete(lv_event_t* e) {
  uint8_t i = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  if (i >= nVisibili) return;
  strncpy(reteScelta, visibili[i].ssid, sizeof(reteScelta) - 1);
  reteScelta[sizeof(reteScelta) - 1] = 0;
  if (visibili[i].protetta) mostraPassword();
  else collegaScelta("");
}

void disegnaReti(bool inCorso) {
  const Tema& t = tema::corrente();
  const Impostazioni& imp = impostazioni::get();
  lv_obj_clean(elencoReti);
  for (uint8_t i = 0; i < nVisibili; i++) {
    const rete::Rete& r = visibili[i];
    lv_obj_t* b = lv_btn_create(elencoReti);
    lv_obj_remove_style_all(b);
    lv_obj_set_size(b, LV_PCT(100), 58);
    lv_obj_set_style_bg_color(b, t.surface2, 0);
    lv_obj_set_style_bg_color(b, lv_color_mix(t.text, t.surface2, 50), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_pad_hor(b, 16, 0);
    lv_obj_set_style_pad_column(b, 12, 0);
    lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_color_t forza = r.rssi >= -60 ? t.ok : (r.rssi >= -75 ? t.accent : t.dangerText);
    ui::etichetta(b, LV_SYMBOL_WIFI, &font_20, forza);
    lv_obj_t* nome = ui::etichetta(b, r.ssid, &font_18, t.text);
    lv_label_set_long_mode(nome, LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(nome, 1);
    if (!strcmp(r.ssid, imp.wifiSsid)) {
      lv_obj_t* c = ui::chip(b, &font_12);
      lv_label_set_text(c, rete::collegata() ? "COLLEGATA" : "SALVATA");
      ui::coloriChip(c, t.accentBg, t.accent);
    }
    lv_obj_t* p = ui::chip(b, &font_12);
    lv_label_set_text(p, r.protetta ? "Protetta" : "Aperta");
    ui::coloriChip(p, t.surface, r.protetta ? t.text : t.text2);
    lv_obj_t* db = ui::etichetta(b, "", &font_14, t.text2);
    lv_label_set_text_fmt(db, "−%d dBm", -r.rssi);
    lv_obj_set_width(db, 76);
    lv_obj_set_style_text_align(db, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_add_event_cb(b, premiRete, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  if (attesaMovimento) return;
  if (inCorso)
    lv_label_set_text(statoReti, "Ricerca in corso… il collegamento con l'antenna si interrompe per qualche secondo.");
  else if (!nVisibili)
    lv_label_set_text(statoReti, "Nessuna rete trovata. Le reti nascoste si inseriscono a mano nel campo «Rete (SSID)».");
  else
    lv_label_set_text_fmt(statoReti, "%u reti trovate · tocca una rete per collegarti", nVisibili);
}

void mostraElenco() {
  const Tema& t = tema::corrente();
  nascondiTastiera();
  lv_obj_clean(boxReti);
  campoPwdRete = btnOcchio = nullptr;
  lv_obj_set_size(boxReti, 700, 520);
  lv_obj_align(boxReti, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t* testa = ui::contenitore(boxReti, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(testa, LV_PCT(100));
  lv_obj_set_flex_align(testa, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* tit = ui::etichetta(testa, "Reti WiFi", &font_24, t.text);
  lv_obj_set_flex_grow(tit, 1);
  lv_obj_t* ag = ui::pulsante(testa, LV_SYMBOL_REFRESH "  Cerca di nuovo", &font_16, ui::Stile::NORMALE);
  lv_obj_set_size(ag, 196, 44);
  lv_obj_add_event_cb(ag, cerca, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* ch = ui::pulsante(testa, "Chiudi", &font_16, ui::Stile::NORMALE);
  lv_obj_set_size(ch, 110, 44);
  lv_obj_add_event_cb(ch, chiudiReti, LV_EVENT_CLICKED, nullptr);

  statoReti = ui::etichetta(boxReti, "", &font_14, t.text2);
  lv_label_set_long_mode(statoReti, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(statoReti, LV_PCT(100));

  elencoReti = ui::contenitore(boxReti, LV_FLEX_FLOW_COLUMN, 6);
  lv_obj_set_width(elencoReti, LV_PCT(100));
  lv_obj_set_flex_grow(elencoReti, 1);
  lv_obj_add_flag(elencoReti, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(elencoReti, LV_DIR_VER);
  versioneVista = 0xFFFFFFFF;  // ridisegna al prossimo aggiornamento
}

void apriReti(lv_event_t*) {
  chiudiReti(nullptr);
  finestra = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(finestra);
  lv_obj_set_size(finestra, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(finestra, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(finestra, LV_OPA_50, 0);
  lv_obj_add_flag(finestra, LV_OBJ_FLAG_CLICKABLE);  // blocca i tocchi sotto
  lv_obj_clear_flag(finestra, LV_OBJ_FLAG_SCROLLABLE);
  boxReti = ui::scheda(finestra);
  lv_obj_set_style_pad_row(boxReti, 10, 0);
  mostraElenco();
  cerca(nullptr);
}

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
  chiudiReti(nullptr);  // finestra di una costruzione precedente (cambio tema)

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
  sezioneAntenna = an;
  char mac[18];
  Collegamento::scriviMac(imp.peer, mac);
  lv_obj_t* l = ui::etichetta(an, "", &font_16, t.text);
  lv_label_set_text_fmt(l, "MAC dell'antenna: %s", mac);
  uint8_t mio[6];
  Collegamento::macLocale(mio);
  Collegamento::scriviMac(mio, mac);
  l = ui::etichetta(an, "", &font_14, t.text2);
  lv_label_set_text_fmt(l, "MAC di questo pannello: %s\nAssociazione: tieni premuto BOOT sull'antenna per 3 s.", mac);
  static const char* const KBIT[] = {"125", "250", "500", "1000"};
  static const uint8_t V_KBIT[] = {0, 1, 2, 3};
  motore_antenna::Scelte& mot = motore_antenna::scelte();
  static const char* const TIPI[] = {"RX · SERVO42D (CAN)", "TX · TMC2209 (STEP/DIR)"};
  static const uint8_t V_TIPI[] = {0, 1};
  ui::etichetta(an, "Tipo di antenna (firmware unico; al cambio l'antenna si riavvia)", &font_14, t.text2);
  gruppoTipo = numGruppi;
  gruppo(an, TIPI, V_TIPI, 2, &mot.tipo, tipoCambiato);
  boxRx = ui::contenitore(an, LV_FLEX_FLOW_COLUMN, 6);
  lv_obj_set_width(boxRx, LV_PCT(100));
  lv_obj_t* rid = ui::contenitore(boxRx, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(rid, LV_PCT(100));
  lv_obj_set_flex_align(rid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* lid = ui::etichetta(rid, "Indirizzo CAN del SERVO42D", &font_14, t.text2);
  lv_obj_set_flex_grow(lid, 1);
  lv_obj_t* meno = ui::pulsante(rid, "−", &font_20, ui::Stile::NORMALE);
  lv_obj_set_size(meno, 52, 44);
  lv_obj_add_event_cb(meno, premiCanId, LV_EVENT_CLICKED, (void*)(intptr_t)-1);
  testoCanId = ui::etichetta(rid, "", &font_20, t.text);
  lv_obj_set_width(testoCanId, 60);
  lv_obj_set_style_text_align(testoCanId, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text_fmt(testoCanId, "%u", mot.canId);
  lv_obj_t* piu = ui::pulsante(rid, "+", &font_20, ui::Stile::NORMALE);
  lv_obj_set_size(piu, 52, 44);
  lv_obj_add_event_cb(piu, premiCanId, LV_EVENT_CLICKED, (void*)(intptr_t)1);
  ui::etichetta(boxRx, "Velocità del bus (kbit/s), uguale a quella del SERVO42D", &font_14, t.text2);
  gruppo(boxRx, KBIT, V_KBIT, 4, &mot.kbitIdx, kbitCambiato);
  statoMotore = ui::etichetta(an, "", &font_14, t.text2);
  lv_label_set_long_mode(statoMotore, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(statoMotore, LV_PCT(100));
  mostraRx();
  lv_obj_t* cal = ui::pulsante(an, "Calibrazione antenna  " LV_SYMBOL_RIGHT, &font_18, ui::Stile::NORMALE);
  lv_obj_set_size(cal, LV_PCT(100), 52);
  lv_obj_add_event_cb(cal, apriCalibrazione, LV_EVENT_CLICKED, nullptr);

  // Manopola remota
  lv_obj_t* mn = sezione(sx, "MANOPOLA REMOTA");
  testoManopola = ui::etichetta(mn, "", &font_14, t.text2);
  lv_label_set_long_mode(testoManopola, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(testoManopola, LV_PCT(100));
  static const char* const SORGENTI[] = {"Microfono", "Segnale di prova"};
  static const uint8_t V_SORGENTI[] = {(uint8_t)proto::SorgenteAudio::MICROFONO, (uint8_t)proto::SorgenteAudio::PROVA};
  link_manopola::Scelte& man = link_manopola::scelte();
  ui::etichetta(mn, "Sorgente del decoder", &font_14, t.text2);
  gruppo(mn, SORGENTI, V_SORGENTI, 2, &man.sorgente, sorgenteCambiata);
  lv_obj_t* rpm = ui::contenitore(mn, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(rpm, LV_PCT(100));
  lv_obj_set_flex_align(rpm, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* lp = ui::etichetta(rpm, "Ritocco fine: passi per scatto", &font_14, t.text2);
  lv_obj_set_flex_grow(lp, 1);
  lv_obj_t* pm = ui::pulsante(rpm, "−", &font_20, ui::Stile::NORMALE);
  lv_obj_set_size(pm, 52, 44);
  lv_obj_add_event_cb(pm, premiPassi, LV_EVENT_CLICKED, (void*)(intptr_t)-1);
  testoPassi = ui::etichetta(rpm, "", &font_20, t.text);
  lv_obj_set_width(testoPassi, 60);
  lv_obj_set_style_text_align(testoPassi, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text_fmt(testoPassi, "%u", man.passiScatto);
  lv_obj_t* pp = ui::pulsante(rpm, "+", &font_20, ui::Stile::NORMALE);
  lv_obj_set_size(pp, 52, 44);
  lv_obj_add_event_cb(pp, premiPassi, LV_EVENT_CLICKED, (void*)(intptr_t)1);

  // QRZ.com
  const ImpostazioniQrz& q = impostazioni::qrz();
  lv_obj_t* qz = sezione(sx, "QRZ.COM");
  lv_obj_t* rq = ui::contenitore(qz, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(rq, LV_PCT(100));
  campoQrzUtente = campo(rq, "Utente", q.utente, sizeof(q.utente) - 1, -1);
  campoQrzPassword = campo(rq, "Password", q.password, sizeof(q.password) - 1, -1, true);
  lv_obj_t* rk = ui::contenitore(qz, LV_FLEX_FLOW_ROW, 10);  // in una riga: nella colonna crescerebbe in altezza
  lv_obj_set_width(rk, LV_PCT(100));
  campoQrzChiave = campo(rk, "Chiave API del logbook (per «I miei QSO»)", q.chiave, sizeof(q.chiave) - 1, -1, true);
  lv_obj_t* nq = ui::etichetta(qz,
                               "Ricerca nominativi: utente e password. I miei QSO: chiave API (sito QRZ → Logbook → "
                               "Settings) e abbonamento «XML Logbook Data».",
                               &font_12, t.text2);
  lv_label_set_long_mode(nq, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(nq, LV_PCT(100));
  lv_obj_t* rp = ui::contenitore(qz, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(rp, LV_PCT(100));
  lv_obj_set_flex_align(rp, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* pa = ui::pulsante(rp, "Prova accesso", &font_16, ui::Stile::SCHEDA);
  lv_obj_set_size(pa, 160, 44);
  lv_obj_add_event_cb(pa, provaQrz, LV_EVENT_CLICKED, nullptr);
  testoQrz = ui::etichetta(rp, "", &font_14, t.text2);
  lv_label_set_long_mode(testoQrz, LV_LABEL_LONG_WRAP);
  lv_obj_set_flex_grow(testoQrz, 1);
  versioneQrz = 0xFFFFFFFF;

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
  ui::etichetta(lr, "Luminosità", &font_14, t.text2);
  testoLuce = ui::etichetta(lr, "", &font_16, t.text);
#if PANNELLO_5B
  lv_label_set_text(testoLuce, "non regolabile sul 5\"");  // CH422G: retroilluminazione solo accesa/spenta
#else
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
#endif
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
  lv_obj_t* pw = ui::contenitore(wf, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(pw, LV_PCT(100));
  lv_obj_t* cr = ui::pulsante(pw, LV_SYMBOL_WIFI "  Cerca reti", &font_16, ui::Stile::PRIMARIO);
  lv_obj_t* ai = ui::pulsante(pw, "Aggiorna indici solari", &font_16, ui::Stile::SCHEDA);
  for (lv_obj_t* b : {cr, ai}) {
    lv_obj_set_height(b, 48);
    lv_obj_set_flex_grow(b, 1);
  }
  lv_obj_add_event_cb(cr, apriReti, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ai, aggiornaIndici, LV_EVENT_CLICKED, nullptr);

  creaTastiera();
}

void aggiorna(const link_antenna::Stato& s) {
  char buf[96];
  // Il tipo vero è quello dichiarato dall'antenna: la scelta lo segue, tranne subito dopo una richiesta di cambio
  // (l'antenna si riavvia in un paio di secondi).
  motore_antenna::Scelte& mot = motore_antenna::scelte();
  bool attesaTipo = cambioTipoAlle && millis() - cambioTipoAlle < 10000;
  if (s.collegato) {
    uint8_t tipoAntenna = (s.r.flag & proto::FLAG_VERSIONE_TX) ? 1 : 0;
    if (tipoAntenna == mot.tipo) {
      cambioTipoAlle = 0;
      attesaTipo = false;
    } else if (!attesaTipo) {
      mot.tipo = tipoAntenna;
      evidenzia(gruppi[gruppoTipo]);
      mostraRx();
    }
  }
  if (attesaTipo) {
    lv_label_set_text(statoMotore, "Cambio del tipo di antenna: l'antenna si riavvia, poi serve un nuovo azzeramento.");
  } else if (!s.collegato) {
    lv_label_set_text(statoMotore, "Antenna non collegata: la scelta verrà inviata quando risponde.");
  } else if (s.r.flag & proto::FLAG_VERSIONE_TX) {
    lv_label_set_text(statoMotore, "Antenna TX: TMC2209 in STEP/DIR, finecorsa sull'ESP32. Calibrazione separata da quella RX.");
  } else {
    lv_label_set_text(statoMotore, "Antenna RX: SERVO42D via CAN, finecorsa sul SERVO42D. Calibrazione separata da quella TX.");
  }
  link_manopola::Stato ms = link_manopola::stato();
  if (!ms.associata)
    lv_label_set_text(testoManopola, "Non associata: accendi la manopola, si presenta da sola e qui compare la richiesta.");
  else if (!ms.collegata)
    lv_label_set_text(testoManopola, "Associata, non collegata (spenta o fuori portata).");
  else
    lv_label_set_text_fmt(testoManopola, "Collegata · segnale %s%d dBm · decoder sulla manopola", ms.rssi < 0 ? "−" : "",
                          ms.rssi < 0 ? -ms.rssi : ms.rssi);

  rete::stato(buf, sizeof(buf));
  char web[72];
  server_web::indirizzo(web, sizeof(web));
  if (web[0]) lv_label_set_text_fmt(testoRete, "%s\nPagina web: %s", buf, web);
  else lv_label_set_text(testoRete, buf);

  uint32_t vq = servizio_qrz::versioneRicerca();
  if (vq != versioneQrz) {
    versioneQrz = vq;
    static servizio_qrz::Ricerca r;  // statica: lo stack del task di LVGL è piccolo
    servizio_qrz::ricerca(r);
    using servizio_qrz::Esito;
    if (r.esito == Esito::IN_CORSO) lv_label_set_text(testoQrz, servizio_qrz::testo(r.esito));
    else if (r.esito == Esito::ERRORE || r.esito == Esito::SENZA_CREDENZIALI || r.esito == Esito::SENZA_RETE)
      lv_label_set_text(testoQrz, r.messaggio[0] ? r.messaggio : servizio_qrz::testo(r.esito));
    else if (r.sessione)
      lv_label_set_text_fmt(testoQrz, r.abbonato ? "Collegato · abbonamento fino a %s" : "Collegato · senza abbonamento%s",
                            r.abbonato ? r.scadenza : "");
    else lv_label_set_text(testoQrz, "");
  }

  if (finestra && elencoReti) {
    rete::Rete r[rete::MAX_RETI];
    bool inCorso;
    uint32_t v;
    uint8_t n = rete::reti(r, rete::MAX_RETI, inCorso, v);
    if (v != versioneVista || inCorso != eraInCorso) {
      versioneVista = v;
      eraInCorso = inCorso;
      memcpy(visibili, r, sizeof(rete::Rete) * n);
      nVisibili = n;
      disegnaReti(inCorso);
    }
  }
}

void mostraReti() { apriReti(nullptr); }

void mostraAntenna() { lv_obj_scroll_to_view(sezioneAntenna, LV_ANIM_OFF); }

void scegliRete(uint8_t indice) {
  lv_event_t e = {};
  e.user_data = (void*)(uintptr_t)indice;
  premiRete(&e);
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
