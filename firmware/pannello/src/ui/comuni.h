// Elementi grafici comuni, con i colori del tema corrente.
// Al cambio tema l'interfaccia viene ricostruita, quindi gli stili sono impostati direttamente.
#pragma once
#include <lvgl.h>

#include <Grafica.h>
#include "../tema.h"

namespace ui {

enum class Stile : uint8_t { NORMALE, PRIMARIO, ATTIVO, PERICOLO, SCHEDA };

lv_obj_t* contenitore(lv_obj_t* parent, lv_flex_flow_t flusso, lv_coord_t spazio = 0);
lv_obj_t* scheda(lv_obj_t* parent);
lv_obj_t* etichetta(lv_obj_t* parent, const char* testo, const lv_font_t* font, lv_color_t colore);
lv_obj_t* titoletto(lv_obj_t* parent, const char* testo);  // "FREQUENZA DA ACCORDARE"
lv_obj_t* pulsante(lv_obj_t* parent, const char* testo, const lv_font_t* font, Stile stile);
void stilePulsante(lv_obj_t* btn, Stile stile);
lv_obj_t* chip(lv_obj_t* parent, const lv_font_t* font);  // etichetta con fondo colorato
void coloriChip(lv_obj_t* chip, lv_color_t fondo, lv_color_t testo);

}  // namespace ui
