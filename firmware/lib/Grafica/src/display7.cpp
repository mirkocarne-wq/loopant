#include "display7.h"

#include <string.h>

namespace display7 {
namespace {

struct Dati {
  char testo[24];
  lv_coord_t h;
  lv_color_t acceso;
  lv_color_t spento;
};

// Cifra in uno spazio 66 x 100, inclinata come nel mockup: x' = x + 10,5 - 0,105 y.
struct P {
  float x, y;
};
inline P sk(float x, float y) { return {x + 10.5f - 0.105f * y, y}; }

void orizzontale(float yc, P out[6]) {
  const float x1 = 6.5f, x2 = 48.5f;
  out[0] = sk(x1, yc);
  out[1] = sk(x1 + 4.5f, yc - 4.5f);
  out[2] = sk(x2 - 4.5f, yc - 4.5f);
  out[3] = sk(x2, yc);
  out[4] = sk(x2 - 4.5f, yc + 4.5f);
  out[5] = sk(x1 + 4.5f, yc + 4.5f);
}

void verticale(float xc, float y1, float y2, P out[6]) {
  out[0] = sk(xc, y1);
  out[1] = sk(xc + 4.5f, y1 + 4.5f);
  out[2] = sk(xc + 4.5f, y2 - 4.5f);
  out[3] = sk(xc, y2);
  out[4] = sk(xc - 4.5f, y2 - 4.5f);
  out[5] = sk(xc - 4.5f, y1 + 4.5f);
}

// Segmenti a, b, c, d, e, f, g
P segmenti[7][6];
bool pronti = false;

void preparaSegmenti() {
  if (pronti) return;
  orizzontale(4.5f, segmenti[0]);             // a
  verticale(50.5f, 7, 48, segmenti[1]);       // b
  verticale(50.5f, 52, 93, segmenti[2]);      // c
  orizzontale(95.5f, segmenti[3]);            // d
  verticale(4.5f, 52, 93, segmenti[4]);       // e
  verticale(4.5f, 7, 48, segmenti[5]);        // f
  orizzontale(50.0f, segmenti[6]);            // g
  pronti = true;
}

// Bit 0 = a ... bit 6 = g
uint8_t maschera(char c) {
  switch (c) {
    case '0': return 0b0111111;
    case '1': return 0b0000110;
    case '2': return 0b1011011;
    case '3': return 0b1001111;
    case '4': return 0b1100110;
    case '5': return 0b1101101;
    case '6': return 0b1111101;
    case '7': return 0b0000111;
    case '8': return 0b1111111;
    case '9': return 0b1101111;
    case '-': return 0b1000000;
    default: return 0;  // '_' = cifra spenta
  }
}

bool cifra(char c) { return (c >= '0' && c <= '9') || c == '-' || c == '_'; }

lv_coord_t larghezzaCarattere(char c, lv_coord_t h) {
  if (cifra(c)) return (lv_coord_t)(h * 0.66f + 0.5f);
  if (c == ',' || c == '.') return (lv_coord_t)(h * 0.22f + 0.5f);
  if (c == ':') return (lv_coord_t)(h * 0.28f + 0.5f);
  return (lv_coord_t)(h * 0.30f + 0.5f);
}

lv_coord_t spaziatura(lv_coord_t h) {
  lv_coord_t g = (lv_coord_t)(h * 0.08f + 0.5f);
  return g < 2 ? 2 : g;
}

lv_coord_t larghezzaTotale(const Dati* d) {
  lv_coord_t w = 0;
  size_t n = strlen(d->testo);
  for (size_t i = 0; i < n; i++) w += larghezzaCarattere(d->testo[i], d->h);
  if (n > 1) w += (lv_coord_t)(n - 1) * spaziatura(d->h);
  return w;
}

void quadratino(lv_draw_ctx_t* ctx, const lv_draw_rect_dsc_t* dsc, lv_coord_t x, lv_coord_t y,
                lv_coord_t lato) {
  lv_area_t a = {x, y, (lv_coord_t)(x + lato - 1), (lv_coord_t)(y + lato - 1)};
  lv_draw_rect(ctx, dsc, &a);
}

void disegna(lv_event_t* e) {
  lv_obj_t* obj = lv_event_get_target(e);
  Dati* d = (Dati*)lv_obj_get_user_data(obj);
  if (!d) return;
  lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);

  lv_draw_rect_dsc_t on, off;
  lv_draw_rect_dsc_init(&on);
  on.bg_color = d->acceso;
  on.bg_opa = LV_OPA_COVER;
  on.radius = 2;
  lv_draw_rect_dsc_init(&off);
  off.bg_color = d->spento;
  off.bg_opa = LV_OPA_COVER;

  const float k = d->h / 100.0f;
  const lv_coord_t gap = spaziatura(d->h);
  lv_coord_t x = area.x1;
  const lv_coord_t y = area.y1;
  const lv_coord_t lato = (lv_coord_t)(12 * k + 0.5f);

  for (const char* p = d->testo; *p; p++) {
    char c = *p;
    if (cifra(c)) {
      uint8_t m = maschera(c);
      for (int s = 0; s < 7; s++) {
        lv_point_t pts[6];
        for (int i = 0; i < 6; i++) {
          pts[i].x = x + (lv_coord_t)(segmenti[s][i].x * k + 0.5f);
          pts[i].y = y + (lv_coord_t)(segmenti[s][i].y * k + 0.5f);
        }
        lv_draw_polygon(ctx, (m & (1 << s)) ? &on : &off, pts, 6);
      }
    } else if (c == ',' || c == '.') {
      quadratino(ctx, &on, x + (lv_coord_t)(4 * k), y + (lv_coord_t)(87 * k), lato);
    } else if (c == ':') {
      quadratino(ctx, &on, x + (lv_coord_t)(11 * k), y + (lv_coord_t)(24 * k), lato);
      quadratino(ctx, &on, x + (lv_coord_t)(7 * k), y + (lv_coord_t)(64 * k), lato);
    }
    x += larghezzaCarattere(c, d->h) + gap;
  }
}

void elimina(lv_event_t* e) {
  lv_obj_t* obj = lv_event_get_target(e);
  void* d = lv_obj_get_user_data(obj);
  if (d) lv_mem_free(d);
  lv_obj_set_user_data(obj, nullptr);
}

void ridimensiona(lv_obj_t* obj, Dati* d) {
  lv_obj_set_size(obj, larghezzaTotale(d), d->h);
  lv_obj_invalidate(obj);
}

}  // namespace

lv_obj_t* crea(lv_obj_t* parent, lv_coord_t altezza, lv_color_t acceso, lv_color_t spento) {
  preparaSegmenti();
  lv_obj_t* obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  Dati* d = (Dati*)lv_mem_alloc(sizeof(Dati));
  memset(d, 0, sizeof(Dati));
  d->h = altezza;
  d->acceso = acceso;
  d->spento = spento;
  lv_obj_set_user_data(obj, d);
  lv_obj_add_event_cb(obj, disegna, LV_EVENT_DRAW_MAIN, nullptr);
  lv_obj_add_event_cb(obj, elimina, LV_EVENT_DELETE, nullptr);
  ridimensiona(obj, d);
  return obj;
}

void testo(lv_obj_t* obj, const char* t) {
  Dati* d = (Dati*)lv_obj_get_user_data(obj);
  if (!d || !strncmp(d->testo, t, sizeof(d->testo))) return;
  strncpy(d->testo, t, sizeof(d->testo) - 1);
  d->testo[sizeof(d->testo) - 1] = 0;
  ridimensiona(obj, d);
}

void colori(lv_obj_t* obj, lv_color_t acceso, lv_color_t spento) {
  Dati* d = (Dati*)lv_obj_get_user_data(obj);
  if (!d) return;
  d->acceso = acceso;
  d->spento = spento;
  lv_obj_invalidate(obj);
}

}  // namespace display7
