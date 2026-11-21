// Manopola (encoder) della scheda: contatore degli scatti, letto dal driver knob di Espressif (bidi_switch_knob).
#pragma once
#include <stdint.h>

namespace encoder {

void inizia();
int32_t scatti();  // contatore dall'accensione: + in senso orario

}  // namespace encoder
