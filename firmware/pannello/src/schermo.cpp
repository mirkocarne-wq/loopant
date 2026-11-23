#include "schermo.h"

#include "waveshare/io_extension.h"
#include "scheda.h"
#include "waveshare/rgb_lcd_port.h"

namespace schermo {

void accendi() { wavesahre_rgb_lcd_bl_on(); }

void spegni() { wavesahre_rgb_lcd_bl_off(); }

void luminosita(uint8_t p) {
#if PANNELLO_5B
  (void)p;  // 5B: il CH422G non ha PWM, retroilluminazione solo accesa/spenta
  return;
#endif
  if (p < 10) p = 10;
  if (p > 100) p = 100;
  IO_EXTENSION_Pwm_Output((uint8_t)(100 - p));
}

void frequenzaPixel(uint32_t hz) { waveshare_rgb_lcd_set_pclk(hz); }

uint32_t frequenzaPixel() { return waveshare_rgb_lcd_get_pclk(); }

void risincronizza(bool attiva) { waveshare_rgb_lcd_set_resync(attiva); }

bool risincronizza() { return waveshare_rgb_lcd_get_resync(); }

uint32_t quadri() { return waveshare_rgb_lcd_frames(); }

}  // namespace schermo
