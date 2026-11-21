// Manopola remota loopant: prima prova dell'hardware (display, touch, manopola, microfono).
#include <Arduino.h>
#include <Grafica.h>
#include <lvgl.h>

#include "config.h"
#include "encoder.h"
#include "microfono.h"
#include "waveshare/cst816.h"
#include "waveshare/lcd_bl_pwm_bsp.h"
#include "waveshare/schermo_lcd.h"

namespace {

lv_obj_t* cifre = nullptr;
lv_obj_t* barra = nullptr;
lv_obj_t* testo = nullptr;

void aggiorna(lv_timer_t*) {
  char t[16];
  snprintf(t, sizeof(t), "%5ld", (long)encoder::scatti());
  for (char* p = t; *p; p++)
    if (*p == ' ') *p = '_';
  display7::testo(cifre, t);
  lv_bar_set_value(barra, (int32_t)(microfono::livello() * 400), LV_ANIM_OFF);  // x4: il parlato è basso
}

void premuto(lv_event_t*) { lv_label_set_text(testo, "tocco ricevuto"); }

void creaInterfaccia() {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B1016), 0);
  lv_obj_t* titolo = lv_label_create(scr);
  lv_label_set_text(titolo, "loopant · manopola");
  lv_obj_set_style_text_color(titolo, lv_color_hex(0xFFB547), 0);
  lv_obj_set_style_text_font(titolo, &font_20, 0);
  lv_obj_align(titolo, LV_ALIGN_TOP_MID, 0, 60);
  cifre = display7::crea(scr, 56, lv_color_hex(0xFFB547), lv_color_hex(0x2A2418));
  lv_obj_align(cifre, LV_ALIGN_CENTER, 0, -10);
  barra = lv_bar_create(scr);
  lv_obj_set_size(barra, 200, 14);
  lv_bar_set_range(barra, 0, 100);
  lv_obj_align(barra, LV_ALIGN_CENTER, 0, 60);
  testo = lv_label_create(scr);
  lv_label_set_text(testo, "gira la manopola, parla, tocca");
  lv_obj_set_style_text_color(testo, lv_color_hex(0xC9D1D9), 0);
  lv_obj_set_style_text_font(testo, &font_16, 0);
  lv_obj_align(testo, LV_ALIGN_CENTER, 0, 100);
  lv_obj_add_event_cb(scr, premuto, LV_EVENT_CLICKED, nullptr);
  lv_timer_create(aggiorna, 50, nullptr);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Touch_Init();
  lcd_lvgl_Init();
  lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
  encoder::inizia();
  bool mic = microfono::inizia(cfg::FREQ_AUDIO_HZ, nullptr);
  if (example_lvgl_lock(-1)) {
    creaInterfaccia();
    example_lvgl_unlock();
  }
  Serial.printf("\nloopant - manopola remota (prova hardware), microfono %s\n", mic ? "attivo" : "NON attivo");
}

void loop() {
  static uint32_t ultimo = 0;
  if (millis() - ultimo > 1000) {
    ultimo = millis();
    Serial.printf("scatti %ld, livello microfono %.3f\n", (long)encoder::scatti(), microfono::livello());
  }
  delay(10);
}
