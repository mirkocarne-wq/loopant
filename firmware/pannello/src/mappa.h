// Widget "mappa del mondo con grey line" (equirettangolare, altezza = larghezza / 2).
// La mappa viene disegnata in un buffer in PSRAM: mare, continenti, coste, reticolo;
// a ogni aggiornamento si aggiungono la notte, la grey line, il sole e la stazione.
#pragma once
#include <lvgl.h>
#include <time.h>

namespace mappa {

lv_obj_t* crea(lv_obj_t* parent, lv_coord_t larghezza, bool citta);

// stazioneValida = false: nessun segno della stazione.
void aggiorna(lv_obj_t* m, time_t t, bool stazioneValida, double lat, double lon, const char* etichetta);

}  // namespace mappa
