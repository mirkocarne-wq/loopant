// Retroilluminazione dello schermo (tramite l'IO expander Waveshare).
#pragma once
#include <stdint.h>

namespace schermo {

void accendi();
void spegni();
// Luminosità 10-100 %. Un valore PWM alto scurisce lo schermo: si scrive 100 - luminosità
// (verificato sulla scheda il 21/11/2026).
void luminosita(uint8_t percento);

// Clock dei pixel e riallineamento del DMA a ogni quadro: servono contro il tremolio orizzontale
// (la PSRAM non sempre fornisce i dati in tempo). Si provano dalla console con "lcd".
void frequenzaPixel(uint32_t hz);  // applicata al prossimo quadro
uint32_t frequenzaPixel();
void risincronizza(bool attiva);
bool risincronizza();
uint32_t quadri();  // quadri trasmessi dall'accensione

}  // namespace schermo
