#include "mappa.h"

#include <esp_heap_caps.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <Grafica.h>
#include "geo.h"
#include "mappa_dati.h"
#include "sole.h"
#include "tema.h"

namespace mappa {
namespace {

struct Dati {
  lv_color_t* base;  // mare, terra, coste, reticolo
  lv_color_t* buf;   // base + notte + grey line (mostrato)
  lv_coord_t w, h;
  lv_obj_t* sole;
  lv_obj_t* stazione;
  lv_obj_t* etStazione;
  lv_obj_t* contatto;  // stazione cercata (QRZ)
  lv_obj_t* etContatto;
  bool contattoValido;
  float cLat, cLon;
};

inline void punto(Dati* d, int x, int y, lv_color_t c, uint8_t mix = 255) {
  if (x < 0 || y < 0 || x >= d->w || y >= d->h) return;
  lv_color_t& p = d->base[y * d->w + x];
  p = mix == 255 ? c : lv_color_mix(c, p, mix);
}

// Linea spessa 2 pixel sull'immagine mostrata (percorso verso la stazione cercata)
void lineaPercorso(Dati* d, int x0, int y0, int x1, int y1, lv_color_t c) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    for (int oy = 0; oy < 2; oy++)
      for (int ox = 0; ox < 2; ox++) {
        int x = x0 + ox, y = y0 + oy;
        if (x >= 0 && y >= 0 && x < d->w && y < d->h) d->buf[y * d->w + x] = c;
      }
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void linea(Dati* d, int x0, int y0, int x1, int y1, lv_color_t c, uint8_t mix = 255) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    punto(d, x0, y0, c, mix);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

float px(Dati* d, float lon) { return (lon + 180.0f) * d->w / 360.0f; }
float py(Dati* d, float lat) { return (90.0f - lat) * d->h / 180.0f; }

// Riempimento per righe di scansione (pari/dispari): funziona anche con poligoni concavi.
void riempi(Dati* d, const mappa_dati::Poligono& p, lv_color_t c) {
  float xs[64];
  for (int y = 0; y < d->h; y++) {
    float yc = y + 0.5f;
    int n = 0;
    for (uint16_t i = 0; i < p.n && n < 64; i++) {
      uint16_t j = (i + 1) % p.n;
      float x1 = px(d, p.punti[2 * i] / 10.0f), y1 = py(d, p.punti[2 * i + 1] / 10.0f);
      float x2 = px(d, p.punti[2 * j] / 10.0f), y2 = py(d, p.punti[2 * j + 1] / 10.0f);
      if ((y1 <= yc && yc < y2) || (y2 <= yc && yc < y1)) xs[n++] = x1 + (yc - y1) * (x2 - x1) / (y2 - y1);
    }
    // ordinamento per inserzione (pochi punti)
    for (int a = 1; a < n; a++) {
      float v = xs[a];
      int b = a - 1;
      while (b >= 0 && xs[b] > v) { xs[b + 1] = xs[b]; b--; }
      xs[b + 1] = v;
    }
    for (int k = 0; k + 1 < n; k += 2) {
      int xa = (int)lroundf(xs[k]), xb = (int)lroundf(xs[k + 1]);
      if (xa < 0) xa = 0;
      if (xb > d->w) xb = d->w;
      for (int x = xa; x < xb; x++) d->base[y * d->w + x] = c;
    }
  }
}

void disegnaBase(Dati* d) {
  const Tema& t = tema::corrente();
  for (int i = 0; i < d->w * d->h; i++) d->base[i] = t.mapSea;
  for (uint8_t k = 0; k < mappa_dati::NUM_POLIGONI; k++) riempi(d, mappa_dati::POLIGONI[k], t.mapLand);
  for (uint8_t k = 0; k < mappa_dati::NUM_POLIGONI; k++) {
    const mappa_dati::Poligono& p = mappa_dati::POLIGONI[k];
    for (uint16_t i = 0; i < p.n; i++) {
      uint16_t j = (i + 1) % p.n;
      linea(d, (int)px(d, p.punti[2 * i] / 10.0f), (int)py(d, p.punti[2 * i + 1] / 10.0f),
            (int)px(d, p.punti[2 * j] / 10.0f), (int)py(d, p.punti[2 * j + 1] / 10.0f), t.mapCoast);
    }
  }
  // reticolo ogni 30 gradi, equatore più marcato
  for (int lon = -150; lon < 180; lon += 30) {
    int x = (int)px(d, (float)lon);
    linea(d, x, 0, x, d->h - 1, t.mapCoast, 60);
  }
  for (int lat = -60; lat <= 60; lat += 30) {
    int y = (int)py(d, (float)lat);
    linea(d, 0, y, d->w - 1, y, t.mapCoast, lat == 0 ? 120 : 60);
  }
}

lv_obj_t* segno(lv_obj_t* parent, lv_coord_t diametro, lv_color_t riempimento, lv_color_t bordo, lv_coord_t spessore) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, diametro, diametro);
  lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(o, riempimento, 0);
  lv_obj_set_style_border_color(o, bordo, 0);
  lv_obj_set_style_border_width(o, spessore, 0);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
  return o;
}

void colloca(lv_obj_t* o, Dati* d, float lat, float lon) {
  lv_coord_t r = lv_obj_get_width(o) / 2;
  lv_obj_set_pos(o, (lv_coord_t)px(d, lon) - r, (lv_coord_t)py(d, lat) - r);
}

void elimina(lv_event_t* e) {
  Dati* d = (Dati*)lv_obj_get_user_data(lv_event_get_target(e));
  if (!d) return;
  heap_caps_free(d->base);
  heap_caps_free(d->buf);
  free(d);
}

}  // namespace

lv_obj_t* crea(lv_obj_t* parent, lv_coord_t w, bool citta) {
  const Tema& t = tema::corrente();
  lv_coord_t h = w / 2;
  size_t dim = (size_t)w * h * sizeof(lv_color_t);
  lv_color_t* base = (lv_color_t*)heap_caps_malloc(dim, MALLOC_CAP_SPIRAM);
  lv_color_t* buf = (lv_color_t*)heap_caps_malloc(dim, MALLOC_CAP_SPIRAM);
  Dati* d = (Dati*)calloc(1, sizeof(Dati));
  if (!base || !buf || !d) {
    heap_caps_free(base);
    heap_caps_free(buf);
    free(d);
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, "Memoria insufficiente per la mappa");
    return l;
  }
  d->base = base;
  d->buf = buf;
  d->w = w;
  d->h = h;

  lv_obj_t* c = lv_canvas_create(parent);
  lv_canvas_set_buffer(c, buf, w, h, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_user_data(c, d);
  lv_obj_add_event_cb(c, elimina, LV_EVENT_DELETE, nullptr);
  disegnaBase(d);
  memcpy(buf, base, dim);

  if (citta) {
    for (uint8_t i = 0; i < mappa_dati::NUM_CITTA; i++) {
      const mappa_dati::Citta& ct = mappa_dati::CITTA[i];
      lv_obj_t* p = segno(c, 6, t.text2, t.text2, 0);
      colloca(p, d, ct.lat, ct.lon);
      lv_obj_t* l = lv_label_create(c);
      lv_label_set_text(l, ct.nome);
      lv_obj_set_style_text_font(l, w >= 800 ? &font_14 : &font_12, 0);
      lv_obj_set_style_text_color(l, t.text2, 0);
      lv_coord_t x = (lv_coord_t)px(d, ct.lon), y = (lv_coord_t)py(d, ct.lat);
      bool sinistra = x > w * 8 / 10;
      lv_obj_update_layout(l);
      lv_obj_set_pos(l, sinistra ? x - 8 - lv_obj_get_width(l) : x + 8, y - 8);
    }
  }
  d->sole = segno(c, 18, t.sun, t.mapSea, 3);
  lv_obj_set_style_shadow_color(d->sole, t.sun, 0);
  lv_obj_set_style_shadow_width(d->sole, 18, 0);
  lv_obj_set_style_shadow_opa(d->sole, LV_OPA_70, 0);
  d->stazione = segno(c, 16, t.mapSea, t.accent, 3);
  d->etStazione = lv_label_create(c);
  lv_obj_set_style_text_font(d->etStazione, &font_14, 0);
  lv_obj_set_style_text_color(d->etStazione, t.accent, 0);
  lv_obj_set_style_bg_color(d->etStazione, t.bg, 0);
  lv_obj_set_style_bg_opa(d->etStazione, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_hor(d->etStazione, 6, 0);
  lv_obj_set_style_radius(d->etStazione, 4, 0);
  lv_obj_add_flag(d->stazione, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(d->etStazione, LV_OBJ_FLAG_HIDDEN);
  if (w < 600) lv_obj_set_style_text_font(d->etStazione, &font_12, 0);

  d->contatto = segno(c, 14, t.mapSea, t.infoText, 3);
  d->etContatto = lv_label_create(c);
  lv_obj_set_style_text_font(d->etContatto, w < 600 ? &font_12 : &font_14, 0);
  lv_obj_set_style_text_color(d->etContatto, t.infoText, 0);
  lv_obj_set_style_bg_color(d->etContatto, t.bg, 0);
  lv_obj_set_style_bg_opa(d->etContatto, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_hor(d->etContatto, 6, 0);
  lv_obj_set_style_radius(d->etContatto, 4, 0);
  lv_obj_add_flag(d->contatto, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(d->etContatto, LV_OBJ_FLAG_HIDDEN);
  return c;
}

void contatto(lv_obj_t* m, bool valido, double lat, double lon, const char* etichetta) {
  Dati* d = (Dati*)lv_obj_get_user_data(m);
  if (!d) return;
  d->contattoValido = valido;
  d->cLat = (float)lat;
  d->cLon = (float)lon;
  if (!valido) {
    lv_obj_add_flag(d->contatto, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(d->etContatto, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(d->contatto, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(d->etContatto, LV_OBJ_FLAG_HIDDEN);
  colloca(d->contatto, d, (float)lat, (float)lon);
  lv_label_set_text(d->etContatto, etichetta);
  lv_obj_update_layout(d->etContatto);
  lv_coord_t x = (lv_coord_t)px(d, (float)lon), y = (lv_coord_t)py(d, (float)lat);
  lv_coord_t lw = lv_obj_get_width(d->etContatto), lh = lv_obj_get_height(d->etContatto);
  lv_coord_t ex = x + lw > d->w ? d->w - lw : x - 4;
  lv_coord_t ey = y - 10 - lh < 0 ? y + 10 : y - 10 - lh;  // sopra il segno, se c'è spazio
  lv_obj_set_pos(d->etContatto, ex < 0 ? 0 : ex, ey);
}

void aggiorna(lv_obj_t* m, time_t t, bool stazioneValida, double lat, double lon, const char* etichetta) {
  Dati* d = (Dati*)lv_obj_get_user_data(m);
  if (!d) return;
  const Tema& tm = tema::corrente();
  memcpy(d->buf, d->base, (size_t)d->w * d->h * sizeof(lv_color_t));

  sole::Posizione p = sole::calcola(t);
  bool notteANord = p.declinazione < 0;
  const int banda = d->h / 60 + 2;  // semi-larghezza della grey line in pixel
  for (int x = 0; x < d->w; x++) {
    double lonX = (x + 0.5) * 360.0 / d->w - 180.0;
    int yT = (int)lround((90.0 - sole::latTerminatore(p, lonX)) * d->h / 180.0);
    for (int y = 0; y < d->h; y++) {
      lv_color_t& c = d->buf[y * d->w + x];
      bool n = notteANord ? y < yT : y > yT;
      if (n) c = lv_color_mix(lv_color_black(), c, tm.mapNotte);
      int dist = abs(y - yT);
      if (dist <= banda) c = lv_color_mix(tm.mapLine, c, (uint8_t)(150 - 110 * dist / banda));
    }
  }
  colloca(d->sole, d, (float)p.declinazione, (float)p.lonSubsolare);

  // percorso sul cerchio massimo verso la stazione cercata
  if (stazioneValida && d->contattoValido) {
    int xp = 0, yp = 0;
    for (int i = 0; i <= 160; i++) {
      double la, lo;
      geo::intermedio(lat, lon, d->cLat, d->cLon, i / 160.0, la, lo);
      int x = (int)px(d, (float)lo), y = (int)py(d, (float)la);
      if (i && abs(x - xp) < d->w / 2) lineaPercorso(d, xp, yp, x, y, tm.infoText);  // salta il bordo della mappa
      xp = x;
      yp = y;
    }
  }

  if (stazioneValida) {
    lv_obj_clear_flag(d->stazione, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(d->etStazione, LV_OBJ_FLAG_HIDDEN);
    colloca(d->stazione, d, (float)lat, (float)lon);
    lv_label_set_text(d->etStazione, etichetta);
    lv_obj_update_layout(d->etStazione);
    lv_coord_t x = (lv_coord_t)px(d, (float)lon), y = (lv_coord_t)py(d, (float)lat);
    lv_coord_t lw = lv_obj_get_width(d->etStazione);
    lv_obj_set_pos(d->etStazione, x + lw > d->w ? d->w - lw : x - 4, y + 10);
  } else {
    lv_obj_add_flag(d->stazione, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(d->etStazione, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_invalidate(m);
}

}  // namespace mappa
