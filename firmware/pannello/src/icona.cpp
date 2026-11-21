#include "icona.h"

#include <math.h>
#include <stdlib.h>

namespace icona {
namespace {

struct Dati {
  Tipo tipo;
  lv_color_t colore;
};

struct Pennello {
  lv_draw_ctx_t* ctx;
  lv_area_t area;
  float k;  // disegni pensati su una griglia 24 x 24
  lv_draw_line_dsc_t linea;
  lv_draw_arc_dsc_t arco;

  lv_point_t p(float x, float y) const {
    return {(lv_coord_t)(area.x1 + x * k + 0.5f), (lv_coord_t)(area.y1 + y * k + 0.5f)};
  }
  void segmento(float x1, float y1, float x2, float y2) {
    lv_point_t a = p(x1, y1), b = p(x2, y2);
    lv_draw_line(ctx, &linea, &a, &b);
  }
  void spezzata(const float* xy, int n) {
    for (int i = 0; i + 1 < n; i++) segmento(xy[2 * i], xy[2 * i + 1], xy[2 * i + 2], xy[2 * i + 3]);
  }
  void cerchio(float cx, float cy, float r) {
    lv_point_t c = p(cx, cy);
    lv_draw_arc(ctx, &arco, &c, (uint16_t)(r * k + 0.5f), 0, 360);
  }
  void ellisse(float cx, float cy, float rx, float ry) {  // come spezzata di 24 lati
    float xy[50];
    for (int i = 0; i <= 24; i++) {
      float a = (float)i / 24 * 6.2831853f;
      xy[2 * i] = cx + rx * cosf(a);
      xy[2 * i + 1] = cy + ry * sinf(a);
    }
    spezzata(xy, 25);
  }
};

void disegna(lv_event_t* e) {
  lv_obj_t* obj = lv_event_get_target(e);
  Dati* d = (Dati*)lv_obj_get_user_data(obj);
  if (!d) return;
  Pennello pn;
  pn.ctx = lv_event_get_draw_ctx(e);
  lv_obj_get_coords(obj, &pn.area);
  pn.k = lv_area_get_width(&pn.area) / 24.0f;
  lv_coord_t spessore = (lv_coord_t)(2 * pn.k + 0.5f);
  if (spessore < 2) spessore = 2;
  lv_draw_line_dsc_init(&pn.linea);
  pn.linea.color = d->colore;
  pn.linea.width = spessore;
  pn.linea.round_start = pn.linea.round_end = 1;
  lv_draw_arc_dsc_init(&pn.arco);
  pn.arco.color = d->colore;
  pn.arco.width = spessore;

  switch (d->tipo) {
    case Tipo::SINTONIA:  // manopola
      pn.cerchio(12, 12, 8);
      pn.segmento(12, 12, 16, 8);
      break;
    case Tipo::MEMORIE: {  // segnalibro
      const float xy[] = {7, 4, 17, 4, 17, 20, 12, 16, 7, 20, 7, 4};
      pn.spezzata(xy, 6);
      break;
    }
    case Tipo::PROPAGAZIONE:  // globo
      pn.cerchio(12, 12, 9);
      pn.segmento(3, 12, 21, 12);
      pn.ellisse(12, 12, 4, 9);
      break;
    case Tipo::DECODER:  // punto, linea, punto
      pn.linea.width = (lv_coord_t)(3.5f * pn.k + 0.5f);
      pn.segmento(2.5f, 12, 4, 12);
      pn.segmento(9, 12, 15, 12);
      pn.segmento(20, 12, 21.5f, 12);
      break;
    case Tipo::DIAGNOSTICA: {  // traccia
      const float xy[] = {3, 12, 7, 12, 10, 5, 14, 19, 17, 12, 21, 12};
      pn.spezzata(xy, 6);
      break;
    }
    case Tipo::IMPOSTAZIONI:  // ingranaggio
      pn.cerchio(12, 12, 3);
      pn.segmento(12, 3, 12, 6);
      pn.segmento(12, 18, 12, 21);
      pn.segmento(3, 12, 6, 12);
      pn.segmento(18, 12, 21, 12);
      pn.segmento(5.6f, 5.6f, 7.7f, 7.7f);
      pn.segmento(16.3f, 16.3f, 18.4f, 18.4f);
      pn.segmento(5.6f, 18.4f, 7.7f, 16.3f);
      pn.segmento(16.3f, 7.7f, 18.4f, 5.6f);
      break;
  }
}

void elimina(lv_event_t* e) {
  lv_obj_t* obj = lv_event_get_target(e);
  free(lv_obj_get_user_data(obj));
  lv_obj_set_user_data(obj, nullptr);
}

}  // namespace

lv_obj_t* crea(lv_obj_t* parent, Tipo tipo, lv_coord_t lato, lv_color_t c) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, lato, lato);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(o, LV_OBJ_FLAG_EVENT_BUBBLE);
  Dati* d = (Dati*)malloc(sizeof(Dati));
  d->tipo = tipo;
  d->colore = c;
  lv_obj_set_user_data(o, d);
  lv_obj_add_event_cb(o, disegna, LV_EVENT_DRAW_MAIN, nullptr);
  lv_obj_add_event_cb(o, elimina, LV_EVENT_DELETE, nullptr);
  return o;
}

void colore(lv_obj_t* o, lv_color_t c) {
  Dati* d = (Dati*)lv_obj_get_user_data(o);
  if (!d || d->colore.full == c.full) return;
  d->colore = c;
  lv_obj_invalidate(o);
}

}  // namespace icona
