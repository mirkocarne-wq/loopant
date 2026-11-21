// Temi del pannello: Notte, Giorno, Fosfori verdi, Fosfori ambra.
// Colori uguali al mockup approvato (docs/software/03-unita-controllo.md, sezione D).
#pragma once
#include <lvgl.h>

struct Tema {
  const char* nome;
  lv_color_t bg, surface, surface2, border, borderStrong;
  lv_color_t text, text2;
  lv_color_t accent, onAccent, accentBg;
  lv_color_t ok, okBg, okText;
  lv_color_t danger, onDanger, dangerBg, dangerText;
  lv_color_t info, infoBg, infoText;
  lv_color_t segOffAccent;  // segmenti spenti del display (accento)
  lv_color_t segOffText;    // segmenti spenti del display (testo)
  // mappa della propagazione
  lv_color_t mapSea, mapLand, mapCoast, mapLine, sun;
  uint8_t mapNotte;  // quanto si scurisce la notte (0-255)
};

namespace tema {

constexpr uint8_t NUMERO = 4;

void inizia();
const Tema& di(uint8_t indice);
const Tema& corrente();
uint8_t indiceCorrente();
void imposta(uint8_t indice);

}  // namespace tema
