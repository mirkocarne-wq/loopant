// Retroilluminazione dello schermo (tramite l'IO expander Waveshare).
#pragma once
#include <stdint.h>

namespace schermo {

void accendi();
void spegni();
// Luminosità 10-100 %. Un valore PWM alto scurisce lo schermo: si scrive 100 - luminosità
// (verificato sulla scheda il 21/11/2026).
void luminosita(uint8_t percento);

}  // namespace schermo
