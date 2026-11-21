// Tocco da remoto (pagina web): un secondo dispositivo di puntamento LVGL accanto al touch GT911.
#pragma once
#include <stdint.h>

namespace tocco_remoto {

void inizia();                     // da chiamare con il lock di LVGL
void tocca(int16_t x, int16_t y);  // pressione breve nel punto (coordinate dello schermo 1024x600)

}  // namespace tocco_remoto
