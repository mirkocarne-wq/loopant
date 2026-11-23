// Menu della manopola: si apre con un tocco lungo nel ritocco fine. Si scorre girando la manopola, tocco breve =
// scegli, tocco lungo = chiudi; si chiude da solo dopo 10 s senza gesti.
// Voci: Decoder (acceso/spento, stato unico con il pannello), Luminosità, Rotazione, Associa a un pannello,
// Informazioni (MAC, canale, pannello, firmware; Dimentica il pannello), Esci.
#pragma once
#include <lvgl.h>
#include <stdint.h>

namespace menu {

void crea(lv_obj_t* scr);   // con il lock di LVGL
void applicaSalvate();      // luminosità e rotazione salvate in NVS ("schermo"), con il lock di LVGL
void apri();
bool aperto();
void scatti(int32_t d);     // manopola girata mentre il menu è aperto
void aggiorna();            // dal timer dell'interfaccia: testi e chiusura automatica

}  // namespace menu
