// Schermate del pannello.
#pragma once
#include <lvgl.h>

#include "../link_antenna.h"

namespace pag_sintonia {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
void proponi(uint32_t hz);       // mostra la frequenza nel display (senza accordare)
uint32_t frequenzaDigitata();
}  // namespace pag_sintonia

namespace pag_impostazioni {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
void mostraReti();               // apre la finestra "Reti WiFi" (anche dal simulatore)
void scegliRete(uint8_t indice);  // come toccare una rete dell'elenco (simulatore)
void mostraAntenna();             // scorre fino alla sezione Antenna (simulatore)
}  // namespace pag_impostazioni

namespace pag_memorie {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
}  // namespace pag_memorie

namespace pag_propagazione {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
void mostrata();
}  // namespace pag_propagazione

namespace pag_decoder {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
}  // namespace pag_decoder

namespace pag_diagnostica {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
}  // namespace pag_diagnostica

namespace pag_calibrazione {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
void mostrata();
}  // namespace pag_calibrazione

// Schermate non ancora realizzate (Memorie, Propagazione, Decoder, Diagnostica)
namespace pag_segnaposto {
void crea(lv_obj_t* contenuto, const char* nome, const char* descrizione);
}  // namespace pag_segnaposto
