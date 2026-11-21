// Schermata QRZ (requisiti Q1-Q6, docs/software/10-qrz.md): ricerca nominativi e "I miei QSO".
#include <bande.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <initializer_list>

#include "../geo.h"
#include "../impostazioni.h"
#include "../locator.h"
#include "../mappa.h"
#include "../rete.h"
#include "../servizio_qrz.h"
#include "../tema.h"
#include "comuni.h"
#include "pagine.h"
#include "ui.h"

namespace pag_qrz {
namespace {

enum Vista : uint8_t { RICERCA, LOGBOOK };
Vista vista = RICERCA;
uint8_t filtro = 0;  // 0 tutte, 40, 20
bool collegato = false;

lv_obj_t* btnVista[2];
lv_obj_t* chipStato;
lv_obj_t* pannelloRicerca;
lv_obj_t* pannelloLogbook;

// Ricerca
lv_obj_t* campoCall;
lv_obj_t* tastiera = nullptr;
lv_obj_t* esitoRicerca;
lv_obj_t* elencoRecenti;
lv_obj_t* vuoto;
lv_obj_t* risultato;
lv_obj_t* lblCall;
lv_obj_t* lblNome;
lv_obj_t* valori[7];
lv_obj_t* mappaObj;
lv_obj_t* notaDati;
uint32_t versioneRicerca = 0xFFFFFFFF;

// Logbook
lv_obj_t* btnFiltri[3];
lv_obj_t* riepilogo;
lv_obj_t* elencoQso;
lv_obj_t* esitoLogbook;
lv_obj_t* zonaMessaggio;
lv_obj_t* intestazione;
uint32_t versioneLogbook = 0xFFFFFFFF;

// Copie dello stato del servizio (statiche: lo stack del task di LVGL è piccolo)
servizio_qrz::Ricerca ricerca;
servizio_qrz::Logbook logbook;

constexpr uint8_t MAX_RECENTI = 5;
char recenti[MAX_RECENTI][16];
uint8_t nRecenti = 0;

const char* ETICHETTE[7] = {"QTH", "Paese", "Locator", "Zone", "QSL", "Distanza", "Direzione"};

void migliaia(long v, char* out, size_t n) {
  if (v >= 1000) snprintf(out, n, "%ld %03ld", v / 1000, v % 1000);
  else snprintf(out, n, "%ld", v);
}

void formattaKhz(uint32_t hz, char* out, size_t n) {
  uint32_t d = (hz + 50) / 100, k = d / 10;
  if (k >= 1000) snprintf(out, n, "%lu %03lu,%lu", (unsigned long)(k / 1000), (unsigned long)(k % 1000), (unsigned long)(d % 10));
  else snprintf(out, n, "%lu,%lu", (unsigned long)k, (unsigned long)(d % 10));
}

bool stazione(double& lat, double& lon) { return locator::aCoordinate(impostazioni::get().locator, lat, lon); }

// --- Vista e intestazione ---------------------------------------------------------------------

void mostraVista(Vista v) {
  vista = v;
  if (v == RICERCA) {
    lv_obj_clear_flag(pannelloRicerca, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(pannelloLogbook, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(pannelloRicerca, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(pannelloLogbook, LV_OBJ_FLAG_HIDDEN);
    if (tastiera) lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
    if (servizio_qrz::versioneLogbook() == 0) servizio_qrz::caricaQso(filtro);  // prima volta
  }
  for (int i = 0; i < 2; i++) ui::stilePulsante(btnVista[i], i == v ? ui::Stile::PRIMARIO : ui::Stile::NORMALE);
}

void premiVista(lv_event_t* e) { mostraVista((Vista)(uintptr_t)lv_event_get_user_data(e)); }

void aggiornaChip() {
  const Tema& t = tema::corrente();
  const ImpostazioniQrz& c = impostazioni::qrz();
  if (!rete::collegata()) {
    lv_label_set_text(chipStato, "WiFi non collegato");
    ui::coloriChip(chipStato, t.dangerBg, t.dangerText);
  } else if (!c.utente[0] || !c.password[0]) {
    lv_label_set_text(chipStato, "Credenziali QRZ in Impostazioni");
    ui::coloriChip(chipStato, t.surface2, t.text2);
  } else if (ricerca.sessione && ricerca.abbonato) {
    lv_label_set_text(chipStato, "Abbonamento QRZ attivo");
    ui::coloriChip(chipStato, t.okBg, t.okText);
  } else if (ricerca.sessione) {
    lv_label_set_text(chipStato, "Senza abbonamento: dati limitati");
    ui::coloriChip(chipStato, t.accentBg, t.accent);
  } else {
    lv_label_set_text(chipStato, "QRZ.com");
    ui::coloriChip(chipStato, t.surface2, t.text2);
  }
}

// --- Ricerca ----------------------------------------------------------------------------------

void avviaRicerca(const char* call) {
  if (!call || !call[0]) return;
  lv_textarea_set_text(campoCall, call);
  if (tastiera) lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_state(campoCall, LV_STATE_FOCUSED);
  servizio_qrz::cerca(call);
}

void premiCerca(lv_event_t*) { avviaRicerca(lv_textarea_get_text(campoCall)); }

void tastieraFatto(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_READY) avviaRicerca(lv_textarea_get_text(campoCall));
  else {
    lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(campoCall, LV_STATE_FOCUSED);
  }
}

void apriTastiera(lv_event_t*) {
  lv_keyboard_set_textarea(tastiera, campoCall);
  lv_keyboard_set_mode(tastiera, LV_KEYBOARD_MODE_TEXT_UPPER);
  lv_obj_clear_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
}

void premiRecente(lv_event_t* e) {
  uint8_t i = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  if (i < nRecenti) avviaRicerca(recenti[i]);
}

void disegnaRecenti() {
  const Tema& t = tema::corrente();
  lv_obj_clean(elencoRecenti);
  for (uint8_t i = 0; i < nRecenti; i++) {
    lv_obj_t* b = ui::pulsante(elencoRecenti, recenti[i], &font_18, ui::Stile::NORMALE);
    lv_obj_set_size(b, LV_PCT(100), 42);
    lv_obj_t* l = lv_obj_get_child(b, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 14, 0);
    lv_obj_add_event_cb(b, premiRecente, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  if (!nRecenti) ui::etichetta(elencoRecenti, "Nessuna ricerca", &font_14, t.text2);
}

void ricorda(const char* call) {
  uint8_t i = 0;
  while (i < nRecenti && strcmp(recenti[i], call)) i++;
  if (i == nRecenti && nRecenti < MAX_RECENTI) nRecenti++;
  if (i >= MAX_RECENTI) i = MAX_RECENTI - 1;
  for (; i > 0; i--) memcpy(recenti[i], recenti[i - 1], sizeof(recenti[0]));
  snprintf(recenti[0], sizeof(recenti[0]), "%s", call);
  disegnaRecenti();
}

void mostraRisultato() {
  const Tema& t = tema::corrente();
  const qrz::Nominativo& n = ricerca.n;
  using servizio_qrz::Esito;

  // messaggio sotto il campo
  if (ricerca.esito == Esito::OK || ricerca.esito == Esito::NESSUNO) {
    lv_label_set_text(esitoRicerca, ricerca.esito == Esito::NESSUNO ? ricerca.messaggio : "");
    lv_obj_set_style_text_color(esitoRicerca, t.text2, 0);
  } else if (ricerca.esito == Esito::IN_CORSO) {
    lv_label_set_text(esitoRicerca, servizio_qrz::testo(ricerca.esito));
    lv_obj_set_style_text_color(esitoRicerca, t.text2, 0);
  } else {
    lv_label_set_text_fmt(esitoRicerca, "%s%s%s", servizio_qrz::testo(ricerca.esito),
                          ricerca.messaggio[0] ? ": " : "", ricerca.messaggio);
    lv_obj_set_style_text_color(esitoRicerca, t.dangerText, 0);
  }
  if (ricerca.esito != Esito::OK) return;

  lv_obj_add_flag(vuoto, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(risultato, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(lblCall, n.call);
  lv_label_set_text(lblNome, n.nome[0] ? n.nome : "—");
  ricorda(n.call);

  char buf[64], a[24];
  lv_label_set_text(valori[0], n.qth[0] ? n.qth : "—");
  if (n.dxcc) snprintf(buf, sizeof(buf), "%s (DXCC %d)", n.paese, n.dxcc);
  else snprintf(buf, sizeof(buf), "%s", n.paese[0] ? n.paese : "—");
  lv_label_set_text(valori[1], buf);
  lv_label_set_text(valori[2], n.locator[0] ? n.locator : "—");
  if (n.cq || n.itu) snprintf(buf, sizeof(buf), "CQ %d · ITU %d", n.cq, n.itu);
  else snprintf(buf, sizeof(buf), "—");
  lv_label_set_text(valori[3], buf);
  lv_label_set_text(valori[4], n.qsl[0] ? n.qsl : "—");

  // distanza e direzione dalla propria stazione
  double la1, lo1, la2 = n.lat, lo2 = n.lon;
  bool io = stazione(la1, lo1);
  bool lui = n.coordinate || locator::aCoordinate(n.locator, la2, lo2);
  if (io && lui) {
    migliaia((long)(geo::distanzaKm(la1, lo1, la2, lo2) + 0.5), a, sizeof(a));
    snprintf(buf, sizeof(buf), "%s km", a);
    lv_label_set_text(valori[5], buf);
    double az = geo::azimut(la1, lo1, la2, lo2);
    snprintf(buf, sizeof(buf), "%.0f° · lungo %.0f°", az, az >= 180 ? az - 180 : az + 180);
    lv_label_set_text(valori[6], buf);
  } else {
    lv_label_set_text(valori[5], io ? "—" : "imposta il tuo locator");
    lv_label_set_text(valori[6], "—");
  }
  mappa::contatto(mappaObj, lui, la2, lo2, n.call);
  const Impostazioni& imp = impostazioni::get();
  mappa::aggiorna(mappaObj, time(nullptr), io, la1, lo1, imp.nominativo[0] ? imp.nominativo : "Stazione");

  if (!ricerca.abbonato)
    lv_label_set_text(notaDati, "Senza abbonamento QRZ alcuni dati non arrivano (locator, coordinate, QSL).");
  else
    lv_label_set_text(notaDati, ricerca.messaggio);
}

// --- Logbook ----------------------------------------------------------------------------------

// Messaggio al centro al posto dell'elenco (o viceversa)
void soloMessaggio(bool si) {
  if (si) {
    lv_obj_add_flag(elencoQso, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(intestazione, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(zonaMessaggio, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(elencoQso, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(intestazione, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(zonaMessaggio, LV_OBJ_FLAG_HIDDEN);
  }
}

void premiFiltro(lv_event_t* e) {
  filtro = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  servizio_qrz::caricaQso(filtro);
}

void premiAggiorna(lv_event_t*) { servizio_qrz::caricaQso(filtro); }

void premiQso(lv_event_t* e) {
  uint16_t i = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
  if (i >= logbook.n) return;
  mostraVista(RICERCA);
  avviaRicerca(logbook.qso[i].call);
}

void premiAccorda(lv_event_t* e) {
  uint16_t i = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
  if (i >= logbook.n) return;
  uint32_t hz = logbook.qso[i].freq_hz;
  bande::Banda b = bande::di(hz);
  if (b == bande::Banda::NESSUNA) return;
  pag_sintonia::proponi(hz);
  if (collegato) {
    link_antenna::invia(proto::Tipo::GOTO, hz);
    Impostazioni& imp = impostazioni::get();
    if (b == bande::Banda::B40M) imp.ultima40_hz = hz;
    else imp.ultima20_hz = hz;
    impostazioni::salva();
  }
  ui::vaiA(ui::SINTONIA);
}

lv_obj_t* cella(lv_obj_t* riga, const char* testo, lv_coord_t larghezza, const lv_font_t* font, lv_color_t colore) {
  lv_obj_t* l = ui::etichetta(riga, testo, font, colore);
  lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
  if (larghezza > 0) lv_obj_set_width(l, larghezza);
  else lv_obj_set_flex_grow(l, 1);
  return l;
}

void mostraLogbookDati() {
  const Tema& t = tema::corrente();
  using servizio_qrz::Esito;
  for (int i = 0; i < 3; i++) {
    const uint8_t f[] = {0, 40, 20};
    ui::stilePulsante(btnFiltri[i], f[i] == filtro ? ui::Stile::ATTIVO : ui::Stile::NORMALE);
  }

  char a[16], b[16];
  if (logbook.esito == Esito::OK) {
    migliaia((long)logbook.totale, a, sizeof(a));
    if (logbook.haConfermati) {
      migliaia((long)logbook.confermati, b, sizeof(b));
      lv_label_set_text_fmt(riepilogo, "%s QSO · %s confermati · ultimi %u", a, b, logbook.n);
    } else {
      lv_label_set_text_fmt(riepilogo, "%s QSO · ultimi %u", a, logbook.n);
    }
  } else {
    lv_label_set_text(riepilogo, "");
  }

  lv_obj_clean(elencoQso);
  if (logbook.esito != Esito::OK || !logbook.n) {
    const char* m = logbook.esito == Esito::OK ? "Nessun QSO" : (logbook.messaggio[0] ? logbook.messaggio : servizio_qrz::testo(logbook.esito));
    if (logbook.esito == Esito::NESSUNO) m = "Premi «Aggiorna» per leggere il logbook";
    if (logbook.esito == Esito::SERVE_ABBONAMENTO)
      lv_label_set_text_fmt(esitoLogbook, "%s.\nLa ricerca dei nominativi funziona anche senza abbonamento.", m);
    else
      lv_label_set_text(esitoLogbook, m);
    lv_obj_set_style_text_color(esitoLogbook,
                                logbook.esito == Esito::ERRORE || logbook.esito == Esito::SERVE_ABBONAMENTO ? t.dangerText : t.text2, 0);
    soloMessaggio(true);
    return;
  }
  soloMessaggio(false);

  for (uint16_t i = 0; i < logbook.n; i++) {
    const qrz::Qso& q = logbook.qso[i];
    lv_obj_t* r = lv_btn_create(elencoQso);
    lv_obj_remove_style_all(r);
    lv_obj_set_size(r, LV_PCT(100), 46);
    lv_obj_set_style_bg_color(r, i % 2 ? t.surface : t.surface2, 0);
    lv_obj_set_style_bg_color(r, lv_color_mix(t.text, t.surface2, 50), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(r, 6, 0);
    lv_obj_set_style_pad_hor(r, 12, 0);
    lv_obj_set_style_pad_column(r, 10, 0);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(r, premiQso, LV_EVENT_CLICKED, (void*)(uintptr_t)i);

    cella(r, q.data, 104, &font_16, t.text2);
    cella(r, q.ora, 56, &font_16, t.text2);
    cella(r, q.call, 0, &font_18, t.text);
    cella(r, q.banda, 60, &font_16, t.text);
    cella(r, q.modo, 70, &font_16, t.text);
    if (q.freq_hz) formattaKhz(q.freq_hz, a, sizeof(a));
    else snprintf(a, sizeof(a), "—");
    cella(r, a, 104, &font_16, t.text);
    snprintf(b, sizeof(b), "%s/%s", q.rstTx[0] ? q.rstTx : "—", q.rstRx[0] ? q.rstRx : "—");
    cella(r, b, 80, &font_14, t.text2);
    cella(r, q.confermato ? LV_SYMBOL_OK : "", 40, &font_16, t.okText);
    if (bande::di(q.freq_hz) != bande::Banda::NESSUNA) {
      lv_obj_t* acc = ui::pulsante(r, "Accorda", &font_14, ui::Stile::NORMALE);
      lv_obj_set_size(acc, 96, 36);
      lv_obj_add_event_cb(acc, premiAccorda, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    } else {
      lv_obj_t* sp = ui::contenitore(r, LV_FLEX_FLOW_ROW, 0);
      lv_obj_set_size(sp, 96, 36);
    }
  }
}

// --- Costruzione ------------------------------------------------------------------------------

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
  lv_obj_set_style_bg_color(tastiera, t.borderStrong, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(tastiera, t.text, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_add_flag(tastiera, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(tastiera, tastieraFatto, LV_EVENT_READY, nullptr);
  lv_obj_add_event_cb(tastiera, tastieraFatto, LV_EVENT_CANCEL, nullptr);
}

void creaRicerca(lv_obj_t* parent) {
  const Tema& t = tema::corrente();
  pannelloRicerca = ui::contenitore(parent, LV_FLEX_FLOW_ROW, 16);
  lv_obj_set_width(pannelloRicerca, LV_PCT(100));
  lv_obj_set_flex_grow(pannelloRicerca, 1);

  lv_obj_t* sx = ui::contenitore(pannelloRicerca, LV_FLEX_FLOW_COLUMN, 10);
  lv_obj_set_size(sx, 290, LV_PCT(100));
  ui::titoletto(sx, "NOMINATIVO");
  lv_obj_t* r = ui::contenitore(sx, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(r, LV_PCT(100));
  campoCall = lv_textarea_create(r);
  lv_textarea_set_one_line(campoCall, true);
  lv_textarea_set_max_length(campoCall, 15);
  lv_textarea_set_accepted_chars(campoCall, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/");
  lv_textarea_set_placeholder_text(campoCall, "Nominativo");
  lv_obj_set_height(campoCall, 52);
  lv_obj_set_flex_grow(campoCall, 1);
  lv_obj_set_style_bg_color(campoCall, t.bg, 0);
  lv_obj_set_style_border_color(campoCall, t.borderStrong, 0);
  lv_obj_set_style_border_color(campoCall, t.accent, LV_STATE_FOCUSED);
  lv_obj_set_style_border_width(campoCall, 2, 0);
  lv_obj_set_style_radius(campoCall, 10, 0);
  lv_obj_set_style_text_color(campoCall, t.text, 0);
  lv_obj_set_style_text_font(campoCall, &font_22, 0);
  lv_obj_add_event_cb(campoCall, apriTastiera, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* bc = ui::pulsante(r, "Cerca", &font_18, ui::Stile::PRIMARIO);
  lv_obj_set_size(bc, 92, 52);
  lv_obj_add_event_cb(bc, premiCerca, LV_EVENT_CLICKED, nullptr);
  esitoRicerca = ui::etichetta(sx, "", &font_14, t.text2);
  lv_label_set_long_mode(esitoRicerca, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(esitoRicerca, LV_PCT(100));
  ui::titoletto(sx, "RICERCHE RECENTI");
  elencoRecenti = ui::contenitore(sx, LV_FLEX_FLOW_COLUMN, 6);
  lv_obj_set_width(elencoRecenti, LV_PCT(100));
  disegnaRecenti();

  lv_obj_t* card = ui::scheda(pannelloRicerca);
  lv_obj_set_height(card, LV_PCT(100));
  lv_obj_set_flex_grow(card, 1);
  vuoto = ui::etichetta(card, "Cerca un nominativo: dati da QRZ.com, distanza, direzione e percorso dalla tua stazione.",
                        &font_18, t.text2);
  lv_label_set_long_mode(vuoto, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(vuoto, LV_PCT(100));

  risultato = ui::contenitore(card, LV_FLEX_FLOW_COLUMN, 10);
  lv_obj_set_size(risultato, LV_PCT(100), LV_PCT(100));
  lv_obj_add_flag(risultato, LV_OBJ_FLAG_HIDDEN);
  lblCall = ui::etichetta(risultato, "", &font_34, t.accent);
  lblNome = ui::etichetta(risultato, "", &font_20, t.text);
  lv_label_set_long_mode(lblNome, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lblNome, LV_PCT(100));
  lv_obj_t* corpo = ui::contenitore(risultato, LV_FLEX_FLOW_ROW, 14);
  lv_obj_set_width(corpo, LV_PCT(100));
  lv_obj_t* info = ui::contenitore(corpo, LV_FLEX_FLOW_COLUMN, 5);
  lv_obj_set_flex_grow(info, 1);
  for (int i = 0; i < 7; i++) {
    lv_obj_t* rr = ui::contenitore(info, LV_FLEX_FLOW_ROW, 8);
    lv_obj_set_width(rr, LV_PCT(100));
    lv_obj_t* et = ui::etichetta(rr, ETICHETTE[i], &font_14, t.text2);
    lv_obj_set_width(et, 78);
    valori[i] = ui::etichetta(rr, "—", &font_16, t.text);
    lv_label_set_long_mode(valori[i], LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(valori[i], 1);
  }
  mappaObj = mappa::crea(corpo, 340, false);
  lv_obj_set_style_radius(mappaObj, 8, 0);
  lv_obj_set_style_clip_corner(mappaObj, true, 0);
  notaDati = ui::etichetta(risultato, "", &font_12, t.text2);
  lv_label_set_long_mode(notaDati, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(notaDati, LV_PCT(100));
}

void creaLogbook(lv_obj_t* parent) {
  const Tema& t = tema::corrente();
  pannelloLogbook = ui::contenitore(parent, LV_FLEX_FLOW_COLUMN, 8);
  lv_obj_set_width(pannelloLogbook, LV_PCT(100));
  lv_obj_set_flex_grow(pannelloLogbook, 1);

  lv_obj_t* su = ui::contenitore(pannelloLogbook, LV_FLEX_FLOW_ROW, 8);
  lv_obj_set_width(su, LV_PCT(100));
  lv_obj_set_flex_align(su, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  const char* nomi[] = {"Tutti", "40 m", "20 m"};
  const uint8_t f[] = {0, 40, 20};
  for (int i = 0; i < 3; i++) {
    btnFiltri[i] = ui::pulsante(su, nomi[i], &font_16, ui::Stile::NORMALE);
    lv_obj_set_size(btnFiltri[i], 92, 42);
    lv_obj_add_event_cb(btnFiltri[i], premiFiltro, LV_EVENT_CLICKED, (void*)(uintptr_t)f[i]);
  }
  riepilogo = ui::etichetta(su, "", &font_16, t.text2);
  lv_obj_set_flex_grow(riepilogo, 1);
  lv_obj_set_style_pad_left(riepilogo, 10, 0);
  lv_obj_t* ag = ui::pulsante(su, LV_SYMBOL_REFRESH "  Aggiorna", &font_16, ui::Stile::NORMALE);
  lv_obj_set_size(ag, 140, 42);
  lv_obj_add_event_cb(ag, premiAggiorna, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* intest = ui::contenitore(pannelloLogbook, LV_FLEX_FLOW_ROW, 10);
  intestazione = intest;
  lv_obj_set_width(intest, LV_PCT(100));
  lv_obj_set_style_pad_hor(intest, 12, 0);
  const char* col[] = {"DATA", "UTC", "NOMINATIVO", "BANDA", "MODO", "kHz", "RST", "QSL"};
  const lv_coord_t lar[] = {104, 56, 0, 60, 70, 104, 80, 40};
  for (int i = 0; i < 8; i++) {
    lv_obj_t* l = ui::titoletto(intest, col[i]);
    if (lar[i]) lv_obj_set_width(l, lar[i]);
    else lv_obj_set_flex_grow(l, 1);
  }
  lv_obj_t* sp = ui::contenitore(intest, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_width(sp, 96);

  elencoQso = ui::contenitore(pannelloLogbook, LV_FLEX_FLOW_COLUMN, 4);
  lv_obj_set_width(elencoQso, LV_PCT(100));
  lv_obj_set_flex_grow(elencoQso, 1);
  lv_obj_add_flag(elencoQso, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(elencoQso, LV_DIR_VER);
  zonaMessaggio = ui::contenitore(pannelloLogbook, LV_FLEX_FLOW_COLUMN, 0);
  lv_obj_set_width(zonaMessaggio, LV_PCT(100));
  lv_obj_set_flex_grow(zonaMessaggio, 1);
  lv_obj_set_flex_align(zonaMessaggio, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  esitoLogbook = ui::etichetta(zonaMessaggio, "", &font_18, t.text2);
  lv_label_set_long_mode(esitoLogbook, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(esitoLogbook, LV_PCT(100));
  lv_obj_set_style_text_align(esitoLogbook, LV_TEXT_ALIGN_CENTER, 0);
}

}  // namespace

void crea(lv_obj_t* c) {
  const Tema& t = tema::corrente();
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 12, 0);
  lv_obj_set_style_pad_top(c, 12, 0);
  lv_obj_set_style_pad_bottom(c, 14, 0);

  lv_obj_t* su = ui::contenitore(c, LV_FLEX_FLOW_ROW, 10);
  lv_obj_set_width(su, LV_PCT(100));
  lv_obj_set_flex_align(su, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  const char* viste[] = {"Ricerca nominativo", "I miei QSO"};
  for (int i = 0; i < 2; i++) {
    btnVista[i] = ui::pulsante(su, viste[i], &font_18, ui::Stile::NORMALE);
    lv_obj_set_size(btnVista[i], 210, 44);
    lv_obj_add_event_cb(btnVista[i], premiVista, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
  lv_obj_t* spazio = ui::contenitore(su, LV_FLEX_FLOW_ROW, 0);
  lv_obj_set_flex_grow(spazio, 1);
  chipStato = ui::chip(su, &font_14);
  ui::coloriChip(chipStato, t.surface2, t.text2);

  creaRicerca(c);
  creaLogbook(c);
  creaTastiera();
  versioneRicerca = versioneLogbook = 0xFFFFFFFF;

  double la, lo;
  bool io = stazione(la, lo);
  const Impostazioni& imp = impostazioni::get();
  mappa::aggiorna(mappaObj, time(nullptr), io, la, lo, imp.nominativo[0] ? imp.nominativo : "Stazione");
  mostraVista(vista);
}

void aggiorna(const link_antenna::Stato& s) {
  collegato = s.collegato;
  uint32_t v = servizio_qrz::versioneRicerca();
  if (v != versioneRicerca) {
    versioneRicerca = v;
    servizio_qrz::ricerca(ricerca);
    mostraRisultato();
  }
  v = servizio_qrz::versioneLogbook();
  if (v != versioneLogbook) {
    versioneLogbook = v;
    servizio_qrz::logbook(logbook);
    if (logbook.esito == servizio_qrz::Esito::IN_CORSO) {
      lv_label_set_text(esitoLogbook, servizio_qrz::testo(logbook.esito));
      lv_obj_set_style_text_color(esitoLogbook, tema::corrente().text2, 0);
      lv_obj_clean(elencoQso);
      soloMessaggio(true);
    } else {
      mostraLogbookDati();
    }
  }
  aggiornaChip();
}

void mostrata() {
  if (vista == LOGBOOK && servizio_qrz::versioneLogbook() == 0) servizio_qrz::caricaQso(filtro);
}

void cerca(const char* nominativo) {
  mostraVista(RICERCA);
  avviaRicerca(nominativo);
}

void mostraLogbook() { mostraVista(LOGBOOK); }

}  // namespace pag_qrz
