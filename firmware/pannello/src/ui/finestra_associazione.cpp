// Finestra "Antenna trovata": compare su qualsiasi scheda quando un'antenna, con il pulsante BOOT premuto,
// chiede di associarsi. Requisito I7 in docs/software/03-unita-controllo.md.
#include <collegamento.h>
#include <stdlib.h>
#include <string.h>

#include <initializer_list>

#include "../impostazioni.h"
#include "comuni.h"
#include "pagine.h"
#include "screensaver.h"

namespace finestra_associazione {
namespace {

lv_obj_t* finestra = nullptr;
uint8_t macMostrato[6] = {0};

void chiudi() {
  if (!finestra) return;
  lv_obj_del(finestra);
  finestra = nullptr;
}

void premiAssocia(lv_event_t*) {
  Impostazioni& imp = impostazioni::get();
  memcpy(imp.peer, macMostrato, 6);
  impostazioni::salva();
  link_antenna::rispondiAssociazione(true);
  chiudi();
}

void premiIgnora(lv_event_t*) {
  link_antenna::rispondiAssociazione(false);
  chiudi();
}

void apri(const link_antenna::Richiesta& r) {
  const Tema& t = tema::corrente();
  memcpy(macMostrato, r.mac, 6);
  finestra = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(finestra);
  lv_obj_set_size(finestra, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(finestra, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(finestra, LV_OPA_50, 0);
  lv_obj_add_flag(finestra, LV_OBJ_FLAG_CLICKABLE);  // blocca i tocchi sotto
  lv_obj_t* box = ui::scheda(finestra);
  lv_obj_set_size(box, 600, LV_SIZE_CONTENT);
  lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_set_style_pad_row(box, 14, 0);
  ui::etichetta(box, "Antenna trovata", &font_24, t.text);

  char mac[18], riga[96];
  Collegamento::scriviMac(r.mac, mac);
  snprintf(riga, sizeof(riga), "L'antenna %s chiede di associarsi a questo pannello.", r.tx ? "TX" : "RX");
  lv_obj_t* l = ui::etichetta(box, riga, &font_20, t.text);
  lv_obj_set_width(l, LV_PCT(100));
  lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
  snprintf(riga, sizeof(riga), "MAC %s · segnale %s%d dBm", mac, r.rssi < 0 ? "−" : "", abs(r.rssi));
  ui::etichetta(box, riga, &font_16, t.text2);

  const Impostazioni& imp = impostazioni::get();
  bool vecchia = false;
  for (int i = 0; i < 6; i++) vecchia |= imp.peer[i] != 0;
  if (vecchia) {
    Collegamento::scriviMac(imp.peer, mac);
    snprintf(riga, sizeof(riga), "Sostituisce l'antenna associata ora (%s).", mac);
    ui::etichetta(box, riga, &font_16, t.text2);
  }
  lv_obj_t* nota = ui::etichetta(box, "Si salva solo il MAC: le chiavi di cifratura restano nel firmware.",
                                 &font_16, t.text2);
  lv_obj_set_width(nota, LV_PCT(100));
  lv_label_set_long_mode(nota, LV_LABEL_LONG_WRAP);

  lv_obj_t* fila = ui::contenitore(box, LV_FLEX_FLOW_ROW, 12);
  lv_obj_set_width(fila, LV_PCT(100));
  lv_obj_set_flex_align(fila, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* no = ui::pulsante(fila, "Ignora", &font_20, ui::Stile::NORMALE);
  lv_obj_t* si = ui::pulsante(fila, "Associa", &font_20, ui::Stile::PRIMARIO);
  for (lv_obj_t* b : {no, si}) lv_obj_set_size(b, 200, 56);
  lv_obj_add_event_cb(no, premiIgnora, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(si, premiAssocia, LV_EVENT_CLICKED, nullptr);
}

}  // namespace

void controlla() {
  link_antenna::Richiesta r = link_antenna::richiestaAssociazione();
  if (!r.presente) {
    chiudi();  // l'antenna ha rinunciato o è stata annullata
    return;
  }
  if (finestra && memcmp(r.mac, macMostrato, 6) == 0) return;
  chiudi();
  screensaver::chiudi();
  apri(r);
}

}  // namespace finestra_associazione
