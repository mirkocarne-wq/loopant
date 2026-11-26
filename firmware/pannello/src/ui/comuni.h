// Elementi grafici comuni, con i colori del tema corrente.
// Al cambio tema l'interfaccia viene ricostruita, quindi gli stili sono impostati direttamente.
#pragma once
#include <bande.h>
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

// Bande dell'antenna associata (la TX solo 40 e 20 m), in ordine di frequenza. Il tipo cambia di rado:
// quando cambia l'interfaccia viene ricostruita, quindi le pagine creano i pulsanti una volta sola.
uint8_t bandeDisponibili(const bande::Info* out[bande::NUMERO]);
bande::Banda bandaDi(uint32_t hz);  // NESSUNA se l'antenna associata non la accorda
const char* testoFuoriBanda();      // "Fuori banda: ..." con le bande dell'antenna

}  // namespace ui
