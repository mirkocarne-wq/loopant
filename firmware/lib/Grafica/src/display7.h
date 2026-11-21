// Widget LVGL "display a 7 segmenti", disegnato con poligoni (nessun font esterno).
// Stessa geometria del mockup: cifre inclinate, segmenti spenti appena visibili.
// Caratteri: 0-9, '-', '_' (cifra tutta spenta), ',' o '.' (punto), ':' e ' ' (spazio).
#pragma once
#include <lvgl.h>

namespace display7 {

lv_obj_t* crea(lv_obj_t* parent, lv_coord_t altezza, lv_color_t acceso, lv_color_t spento);
void testo(lv_obj_t* obj, const char* testo);
void colori(lv_obj_t* obj, lv_color_t acceso, lv_color_t spento);

}  // namespace display7
