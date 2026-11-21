// Icone delle schede, disegnate con linee e archi LVGL (come nel mockup approvato).
#pragma once
#include <lvgl.h>

namespace icona {

enum class Tipo : uint8_t { SINTONIA, MEMORIE, PROPAGAZIONE, DECODER, DIAGNOSTICA, IMPOSTAZIONI };

lv_obj_t* crea(lv_obj_t* parent, Tipo tipo, lv_coord_t lato, lv_color_t colore);
void colore(lv_obj_t* icona, lv_color_t colore);

}  // namespace icona
