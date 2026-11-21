// Microfono PDM della manopola: I2S0 in modo PDM a 4 volte la frequenza dei decoder, componente continua tolta,
// FIR passa-basso e riduzione (Decoder::Decimatore), campioni a 16 bit consegnati da un task sul core 0.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace microfono {

typedef void (*Consumatore)(const int16_t* campioni, size_t n);

bool inizia(uint32_t fs, Consumatore consumatore);
bool attivo();
float livello();  // ampiezza efficace recente, 0-1 del fondo scala

}  // namespace microfono
