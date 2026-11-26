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
#include "src/copia_sd.h"
#include "src/decodifica.h"
#include "src/eventi.h"
#include "src/impostazioni.h"
#include "src/link_antenna.h"
#include "src/scheda.h"
#include "src/link_manopola.h"
#include "src/motore_antenna.h"
#include "src/ora.h"
#include "src/rete.h"
#include "src/schermo.h"
#include "src/server_web.h"
#include "src/servizio_qrz.h"
#include "src/tocco_remoto.h"
#include "src/tema.h"
#include "src/ui/ui.h"
#include "src/waveshare/io_extension.h"
#include "src/waveshare/lvgl_port.h"

void setup() {
  Serial.begin(115200);  // USB nativa (con "USB CDC On Boot") oppure USB TO UART
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);  // USB TO UART (CH343): la console risponde su entrambe le porte
#endif
  impostazioni::carica();
  motore_antenna::carica();
  ora::applicaFuso();
  tema::inizia();

  // Schermo, touch e LVGL come negli esempi Waveshare
  esp_lcd_touch_handle_t touch = touch_gt911_init();
  // GPIO19/20 alla USB nativa (EXIO5 basso; alto = CAN, non usato sul pannello)
#if !PANNELLO_5B
  IO_EXTENSION_Output(IO_EXTENSION_IO_5, 0);  // 7B: USB su GPIO 19/20 (non CAN); il 5B ha il CAN su 15/16
#endif
  esp_lcd_panel_handle_t schermo = waveshare_esp32_s3_rgb_lcd_init();
  wavesahre_rgb_lcd_bl_on();
  schermo::luminosita(impostazioni::get().luminosita);
  ESP_ERROR_CHECK(lvgl_port_init(schermo, touch));

  link_antenna::inizia(impostazioni::get().peer);
  link_manopola::inizia();  // dopo l'antenna: ESP-NOW è già acceso
  rete::inizia();
  servizio_qrz::inizia();
  server_web::inizia();
  decodifica::inizia();

  if (lvgl_port_lock(-1)) {
    ui::inizia();
    tocco_remoto::inizia();
    lvgl_port_unlock();
  }
  console::inizia();
}

void loop() {
  link_antenna::aggiorna();
  link_manopola::aggiorna();
  eventi::osserva(link_antenna::stato());
  console::aggiorna();
  copia_sd::aggiorna();  // copia e ripristino su microSD
  delay(5);
}
