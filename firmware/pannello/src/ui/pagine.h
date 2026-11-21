// Schermate del pannello.
#pragma once
#include <lvgl.h>

#include "../link_antenna.h"

namespace pag_sintonia {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
}  // namespace pag_sintonia

namespace pag_impostazioni {
void crea(lv_obj_t* contenuto);
void aggiorna(const link_antenna::Stato& s);
}  // namespace pag_impostazioni

// Schermate non ancora realizzate (Memorie, Propagazione, Decoder, Diagnostica)
namespace pag_segnaposto {
void crea(lv_obj_t* contenuto, const char* nome, const char* descrizione);
}  // namespace pag_segnaposto
