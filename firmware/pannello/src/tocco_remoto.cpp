#include "tocco_remoto.h"

#include <Arduino.h>
#include <lvgl.h>

namespace tocco_remoto {
namespace {

constexpr uint32_t DURATA_MS = 150;  // quanto dura la pressione
volatile int16_t px = 0, py = 0;
volatile uint32_t fine = 0;
volatile bool attivo = false;

void leggi(lv_indev_drv_t*, lv_indev_data_t* d) {
  d->point.x = px;
  d->point.y = py;
  bool premuto = attivo && (int32_t)(fine - millis()) > 0;
  if (!premuto) attivo = false;
  d->state = premuto ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

}  // namespace

void inizia() {
  static lv_indev_drv_t drv;
  lv_indev_drv_init(&drv);
  drv.type = LV_INDEV_TYPE_POINTER;
  drv.read_cb = leggi;
  lv_indev_drv_register(&drv);
}

void tocca(int16_t x, int16_t y) {
  if (x < 0) x = 0;
  if (x > 1023) x = 1023;
  if (y < 0) y = 0;
  if (y > 599) y = 599;
  px = x;
  py = y;
  fine = millis() + DURATA_MS;
  attivo = true;
}

}  // namespace tocco_remoto
