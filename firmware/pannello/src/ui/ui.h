// Interfaccia del pannello: barra in alto, schede in basso, schermate.
// Struttura e requisiti: docs/software/03-unita-controllo.md
#pragma once
#include <lvgl.h>

#include "../link_antenna.h"

namespace ui {

// Da chiamare con il lock di LVGL (lvgl_port_lock).
void inizia();

// Richiede la ricostruzione dell'interfaccia (cambio tema, nominativo...).
// Si può chiamare da qualunque task: la ricostruzione avviene nel task di LVGL.
void richiediRicostruzione();

// Interfaccia di una schermata.
struct Pagina {
  const char* nome;
  const char* simbolo;
  void (*crea)(lv_obj_t* contenuto);
  void (*aggiorna)(const link_antenna::Stato& s);
};

}  // namespace ui
