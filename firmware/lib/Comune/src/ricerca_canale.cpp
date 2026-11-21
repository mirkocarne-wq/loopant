#include "ricerca_canale.h"

#include <esp_wifi.h>

namespace ricerca_canale {

bool cerca(Collegamento& radio, uint8_t& canale, const void* pacchetto, size_t len, uint32_t ackMs) {
  for (uint8_t i = 0; i < 13; i++) {
    uint8_t ch = (canale - 1 + i) % 13 + 1;
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    for (int tentativo = 0; tentativo < 2; tentativo++) {
      if (radio.inviaConConferma(pacchetto, len, ackMs)) {
        canale = ch;
        return true;
      }
    }
  }
  esp_wifi_set_channel(canale, WIFI_SECOND_CHAN_NONE);
  return false;
}

}  // namespace ricerca_canale
