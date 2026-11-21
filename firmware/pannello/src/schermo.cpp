#include "schermo.h"

#include "waveshare/io_extension.h"
#include "waveshare/rgb_lcd_port.h"

namespace schermo {

void accendi() { wavesahre_rgb_lcd_bl_on(); }

void spegni() { wavesahre_rgb_lcd_bl_off(); }

void luminosita(uint8_t p) {
  if (p < 10) p = 10;
  if (p > 100) p = 100;
  IO_EXTENSION_Pwm_Output((uint8_t)(100 - p));
}

}  // namespace schermo
