// Tabelle di calibrazione posizione <-> frequenza, una per banda, salvate in NVS.
// Interpolazione lineare in 1/f². Vedi docs/software/05-tuning-calibrazione.md
#pragma once
#include <Print.h>
#include <stdint.h>

#include "bande.h"

namespace calibrazione {

constexpr uint8_t MAX_PUNTI = 16;
constexpr uint32_t TOLLERANZA_HZ = 1000;  // un punto entro 1 kHz sostituisce quello esistente

void inizia();  // carica dalla flash
bool salvaPunto(uint32_t freq_hz, int32_t posizione);
void cancella(bande::Banda b);
bool calibrata(bande::Banda b);  // almeno 2 punti
bool posizionePer(uint32_t freq_hz, int32_t& posizione);
void stampa(Print& out);

}  // namespace calibrazione
