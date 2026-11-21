// Ingresso audio dalla radio: ADC continuo dell'ESP32-S3 (IDF 5), 12 bit, centrato e portato a 16 bit.
// Collegamento consigliato: docs/tecnico/06-elettronica.md (condensatore, partitore a metà tensione).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace audio {

typedef void (*Consumatore)(const int16_t* campioni, size_t n);

// false se il pin è -1, non è un ingresso ADC o il driver non parte.
bool inizia(int gpio, uint32_t fs, Consumatore consumatore);
bool attivo();
float livello();  // ampiezza efficace recente, 0-1 del fondo scala

}  // namespace audio
