// Retroilluminazione dello schermo (tramite l'IO expander Waveshare).
#pragma once
#include <stdint.h>

namespace schermo {

void accendi();
void spegni();
// Luminosità 10-100 %. Negli esempi Waveshare un valore PWM alto scurisce lo schermo:
// qui si scrive 100 - luminosità. Verso da verificare sulla scheda.
void luminosita(uint8_t percento);

}  // namespace schermo
