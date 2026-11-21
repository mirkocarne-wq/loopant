// Interfaccia del pannello: barra in alto, schede in basso, schermate.
// Struttura e requisiti: docs/software/03-unita-controllo.md
#pragma once
#include <lvgl.h>

#include "../link_antenna.h"

namespace ui {

// Indici delle schermate. Le prime NUM_SCHEDE hanno una scheda in basso.
enum Pag : uint8_t {
  SINTONIA = 0,
  MEMORIE,
  PROPAGAZIONE,
  DECODER,
  QRZ,
  DIAGNOSTICA,
  IMPOSTAZIONI,
  CALIBRAZIONE,  // si apre da Impostazioni
  NUM_PAGINE
};
constexpr uint8_t NUM_SCHEDE = CALIBRAZIONE;

// Da chiamare con il lock di LVGL (lvgl_port_lock).
void inizia();

// Richiede la ricostruzione dell'interfaccia (cambio tema, nominativo...).
// Si può chiamare da qualunque task: la ricostruzione avviene nel task di LVGL.
void richiediRicostruzione();

// Mostra una schermata (dal task di LVGL).
void vaiA(uint8_t pagina);

// Interfaccia di una schermata.
struct Pagina {
  const char* nome;
  const char* simbolo;
  void (*crea)(lv_obj_t* contenuto);
  void (*aggiorna)(const link_antenna::Stato& s);
  void (*mostrata)();  // chiamata quando la schermata diventa visibile (può essere nullptr)
};

}  // namespace ui
