#include "encoder.h"

#include "config.h"
#include "waveshare/bidi_switch_knob.h"

namespace encoder {
namespace {
volatile int32_t conta = 0;
knob_handle_t manopola = nullptr;
void destra(void*, void*) { conta = conta + 1; }
void sinistra(void*, void*) { conta = conta - 1; }
}  // namespace

void inizia() {
  knob_config_t c = {};
  c.gpio_encoder_a = cfg::PIN_ENCODER_A;
  c.gpio_encoder_b = cfg::PIN_ENCODER_B;
  manopola = iot_knob_create(&c);
  if (!manopola) return;
  iot_knob_register_cb(manopola, KNOB_RIGHT, destra, nullptr);
  iot_knob_register_cb(manopola, KNOB_LEFT, sinistra, nullptr);
}

int32_t scatti() { return conta; }

}  // namespace encoder
