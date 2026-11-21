// Ingresso audio dalla radio: ADC continuo dell'ESP32-S3 (IDF 5), 12 bit, a 4 volte la frequenza richiesta;
// componente continua tolta, FIR passa-basso e riduzione alla frequenza del decoder (decoder/decimatore.h),
// campioni a 16 bit. Circuito d'ingresso: docs/tecnico/06-elettronica.md (connettore J8, GPIO6).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace audio {

typedef void (*Consumatore)(const int16_t* campioni, size_t n);

// fs: frequenza dei campioni consegnati (l'ADC lavora a 4 fs). false se il pin è -1, non è un ingresso ADC
// o il driver non parte.
bool inizia(int gpio, uint32_t fs, Consumatore consumatore);
bool attivo();
float livello();  // ampiezza efficace recente, 0-1 del fondo scala

}  // namespace audio
