// loopant - pannello in casa (Waveshare ESP32-S3-Touch-LCD-7B, 1024x600).
// Requisiti: docs/software/03-unita-controllo.md. Build: firmware/pannello/README.md
//
// Il task di LVGL (driver Waveshare) gira sul core 1; il loop() gestisce il collegamento
// con l'antenna e la console seriale. L'interfaccia si modifica solo dal task di LVGL
// o con lvgl_port_lock().
#include <Arduino.h>
#include <bande.h>
#include <collegamento.h>
#include <protocollo.h>

#include "src/console.h"
#include "src/eventi.h"
#include "src/impostazioni.h"
#include "src/link_antenna.h"
#include "src/tema.h"
#include "src/ui/ui.h"
#include "src/waveshare/lvgl_port.h"

void setup() {
  Serial.begin(115200);
  impostazioni::carica();
  tema::inizia();

  // Schermo, touch e LVGL come negli esempi Waveshare
  esp_lcd_touch_handle_t touch = touch_gt911_init();
  esp_lcd_panel_handle_t schermo = waveshare_esp32_s3_rgb_lcd_init();
  wavesahre_rgb_lcd_bl_on();
  ESP_ERROR_CHECK(lvgl_port_init(schermo, touch));

  link_antenna::inizia(impostazioni::get().peer);

  if (lvgl_port_lock(-1)) {
    ui::inizia();
    lvgl_port_unlock();
  }
  console::inizia();
}

void loop() {
  link_antenna::aggiorna();
  eventi::osserva(link_antenna::stato());
  console::aggiorna();
  delay(5);
}
