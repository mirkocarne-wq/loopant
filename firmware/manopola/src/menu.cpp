#include "menu.h"

#include <Arduino.h>
#include <Grafica.h>
#include <Preferences.h>
#include <collegamento.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "condiviso.h"
#include "decodifica.h"
#include "waveshare/lcd_bl_pwm_bsp.h"
#include "waveshare/schermo_lcd.h"

namespace menu {
namespace {

const lv_color_t FONDO = lv_color_hex(0x0B1016);
const lv_color_t TESTO = lv_color_hex(0xE6EDF3);
const lv_color_t TESTO2 = lv_color_hex(0x8B949E);
const lv_color_t ACCENTO = lv_color_hex(0xFFB547);

constexpr uint32_t CHIUSURA_MS = 10000;
constexpr int RIGHE = 6;
const uint8_t LUCE_PERCENTO[] = {25, 50, 75, 100};

enum class Vista : uint8_t { CHIUSO, PRINCIPALE, INFO };
enum Voce { DECODER, LUCE, ROTAZIONE, ASSOCIA, INFORMAZIONI, ESCI };

lv_obj_t* pannello = nullptr;  // sovrapposto a tutto lo schermo
lv_obj_t* titolo = nullptr;
lv_obj_t* righe[RIGHE] = {};
lv_obj_t* info = nullptr;      // testo della vista Informazioni
char testi[RIGHE][48] = {};
Vista vista = Vista::CHIUSO;
int scelta = 0;
uint32_t ultimoGesto = 0;
bool confermaDimentica = false;

// Preferenze dello schermo (NVS "schermo")
uint8_t luce = 3;  // indice in LUCE_PERCENTO
bool ruota180 = true;

void salva() {
  Preferences p;
  p.begin("schermo", false);
  p.putUChar("luce", luce);
  p.putBool("r180", ruota180);
  p.end();
}

void applicaLuce() { setUpdutySubdivide((uint16_t)(LUCE_PERCENTO[luce] * 255 / 100)); }

void applicaRotazione() {
  schermo_ruota_180(ruota180);
  lv_obj_invalidate(lv_scr_act());  // tutto da ridisegnare con il nuovo orientamento
}

int numeroVoci() { return vista == Vista::INFO ? 2 : RIGHE; }

void dataCompilazione(char* out, size_t n) {  // __DATE__ = "Nov 23 2026" (giorno a una cifra: "Nov  3 2026")
  const char* d = __DATE__;
  static const char* MESI = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int mese = 1;
  for (int m = 0; m < 12; m++)
    if (!strncmp(MESI + 3 * m, d, 3)) mese = m + 1;
  snprintf(out, n, "%02d/%02d/%s", atoi(d + 4), mese, d + 7);
}

void chiudi() {
  vista = Vista::CHIUSO;
  confermaDimentica = false;
  if (pannello) lv_obj_add_flag(pannello, LV_OBJ_FLAG_HIDDEN);
}

void testo(int i, const char* t) {  // aggiorna una riga solo se è cambiata
  if (strncmp(testi[i], t, sizeof(testi[i])) == 0) return;
  snprintf(testi[i], sizeof(testi[i]), "%s", t);
  lv_label_set_text(righe[i], testi[i]);
}

void ridisegna() {
  condiviso::Pannello p = condiviso::pannello();
  char t[48];
  for (int i = 0; i < RIGHE; i++) {
    bool visibile = i < numeroVoci();
    if (visibile) lv_obj_clear_flag(righe[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(righe[i], LV_OBJ_FLAG_HIDDEN);
    bool sel = i == scelta;
    lv_obj_set_style_bg_opa(righe[i], sel ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(righe[i], sel ? FONDO : TESTO, 0);
  }
  if (vista == Vista::PRINCIPALE) {
    lv_label_set_text(titolo, "MENU");
    lv_obj_add_flag(info, LV_OBJ_FLAG_HIDDEN);
    snprintf(t, sizeof(t), "Decoder: %s%s", decodifica::attivo() ? "acceso" : "spento",
             p.collegato ? "" : " (pannello assente)");
    testo(DECODER, t);
    snprintf(t, sizeof(t), "Luminosità: %u %%", LUCE_PERCENTO[luce]);
    testo(LUCE, t);
    snprintf(t, sizeof(t), "Rotazione: %s", ruota180 ? "180°" : "0°");
    testo(ROTAZIONE, t);
    testo(ASSOCIA, "Associa a un pannello");
    testo(INFORMAZIONI, "Informazioni");
    testo(ESCI, "Esci");
    for (int i = 0; i < RIGHE; i++) lv_obj_align(righe[i], LV_ALIGN_CENTER, 0, -88 + i * 38);
  } else if (vista == Vista::INFO) {
    lv_label_set_text(titolo, "INFORMAZIONI");
    lv_obj_clear_flag(info, LV_OBJ_FLAG_HIDDEN);
    uint8_t mio[6];
    char mac[18], data[12], riga[200];
    Collegamento::macLocale(mio);
    Collegamento::scriviMac(mio, mac);
    dataCompilazione(data, sizeof(data));
    const char* pann = p.associato ? p.macPannello : "nessuno";
    if (p.collegato)
      snprintf(riga, sizeof(riga), "Manopola %s\nPannello %s\nCanale %u · %s%d dBm\nFirmware del %s", mac, pann,
               p.canale, p.rssi < 0 ? "−" : "", p.rssi < 0 ? -p.rssi : p.rssi, data);
    else
      snprintf(riga, sizeof(riga), "Manopola %s\nPannello %s\nCanale %u · non collegata\nFirmware del %s", mac, pann,
               p.canale, data);
    static char infoMostrata[200];
    if (strcmp(riga, infoMostrata)) {  // ridisegna solo se cambia
      snprintf(infoMostrata, sizeof(infoMostrata), "%s", riga);
      lv_label_set_text(info, infoMostrata);
    }
    testo(0, confermaDimentica ? "Tocca ancora: dimentica" : "Dimentica il pannello");
    testo(1, "Indietro");
    lv_obj_align(righe[0], LV_ALIGN_CENTER, 0, 58);
    lv_obj_align(righe[1], LV_ALIGN_CENTER, 0, 96);
  }
}

void scegli() {
  ultimoGesto = millis();
  condiviso::Comandi c = condiviso::comandi();
  if (vista == Vista::INFO) {
    if (scelta == 0) {
      if (!confermaDimentica) {
        confermaDimentica = true;
      } else {
        c.nDimentica++;
        condiviso::comandi(c);
        chiudi();
        return;
      }
    } else {
      vista = Vista::PRINCIPALE;
      scelta = INFORMAZIONI;
      confermaDimentica = false;
    }
    ridisegna();
    return;
  }
  switch (scelta) {
    case DECODER:
      if (!condiviso::pannello().collegato) break;  // lo stato lo tiene il pannello
      c.decoderVoluto = decodifica::attivo() ? 0 : 1;
      c.nDecoder++;
      condiviso::comandi(c);
      break;
    case LUCE:
      luce = (luce + 1) % sizeof(LUCE_PERCENTO);
      applicaLuce();
      salva();
      break;
    case ROTAZIONE:
      ruota180 = !ruota180;
      applicaRotazione();
      salva();
      break;
    case ASSOCIA:
      c.nAssocia++;  // la ricerca la fa il loop; l'avviso "Cerco il pannello" lo mostra l'interfaccia
      condiviso::comandi(c);
      chiudi();
      return;
    case INFORMAZIONI:
      vista = Vista::INFO;
      scelta = 1;  // su "Indietro": "Dimentica" va scelto apposta
      break;
    default:
      chiudi();
      return;
  }
  ridisegna();
}

void evento(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED) scegli();
  else if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) chiudi();
}

}  // namespace

void crea(lv_obj_t* scr) {
  pannello = lv_obj_create(scr);
  lv_obj_remove_style_all(pannello);
  lv_obj_set_size(pannello, 360, 360);
  lv_obj_center(pannello);
  lv_obj_set_style_bg_color(pannello, FONDO, 0);
  lv_obj_set_style_bg_opa(pannello, LV_OPA_COVER, 0);
  lv_obj_add_flag(pannello, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(pannello, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(pannello, evento, LV_EVENT_SHORT_CLICKED, nullptr);
  lv_obj_add_event_cb(pannello, evento, LV_EVENT_LONG_PRESSED, nullptr);

  titolo = lv_label_create(pannello);
  lv_obj_set_style_text_font(titolo, &font_18, 0);
  lv_obj_set_style_text_color(titolo, ACCENTO, 0);
  lv_obj_align(titolo, LV_ALIGN_CENTER, 0, -128);

  info = lv_label_create(pannello);
  lv_obj_set_style_text_font(info, &font_16, 0);
  lv_obj_set_style_text_color(info, TESTO2, 0);
  lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_line_space(info, 4, 0);
  lv_obj_align(info, LV_ALIGN_CENTER, 0, -36);

  for (int i = 0; i < RIGHE; i++) {
    righe[i] = lv_label_create(pannello);
    lv_obj_set_size(righe[i], 250, 34);
    lv_obj_set_style_text_font(righe[i], &font_20, 0);
    lv_obj_set_style_text_align(righe[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(righe[i], 4, 0);
    lv_obj_set_style_radius(righe[i], 8, 0);
    lv_obj_set_style_bg_color(righe[i], ACCENTO, 0);
    lv_label_set_long_mode(righe[i], LV_LABEL_LONG_CLIP);
    lv_label_set_text(righe[i], "");
  }
  lv_obj_add_flag(pannello, LV_OBJ_FLAG_HIDDEN);
}

void applicaSalvate() {
  Preferences p;
  p.begin("schermo", true);
  luce = p.getUChar("luce", 3);
  ruota180 = p.getBool("r180", true);
  p.end();
  if (luce >= sizeof(LUCE_PERCENTO)) luce = 3;
  applicaLuce();
  applicaRotazione();
}

void apri() {
  vista = Vista::PRINCIPALE;
  scelta = 0;
  confermaDimentica = false;
  ultimoGesto = millis();
  for (auto& t : testi) t[0] = 0;
  lv_obj_clear_flag(pannello, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(pannello);
  ridisegna();
}

bool aperto() { return vista != Vista::CHIUSO; }

void scatti(int32_t d) {
  ultimoGesto = millis();
  scelta += d > 0 ? 1 : -1;
  if (scelta < 0) scelta = 0;
  if (scelta >= numeroVoci()) scelta = numeroVoci() - 1;
  confermaDimentica = false;
  ridisegna();
}

void aggiorna() {
  if (!aperto()) return;
  if (millis() - ultimoGesto > CHIUSURA_MS) {
    chiudi();
    return;
  }
  ridisegna();  // stato del decoder, collegamento e dBm possono cambiare
}

}  // namespace menu
