// Registro degli eventi e contatori per la schermata Diagnostica.
// Gli eventi si ricavano dai cambi di stato dell'antenna osservati dal pannello.
#pragma once
#include <stdint.h>

#include "link_antenna.h"

namespace eventi {

enum class Tipo : uint8_t { ACCORDATA, ZERO, ERRORE, COLLEGAMENTO };

struct Evento {
  char ora[9];     // "14:31" (UTC) oppure "+12:05" (tempo dall'accensione)
  Tipo tipo;
  char testo[56];
};

struct Contatori {
  uint32_t movimenti;
  uint32_t azzeramenti;
  uint32_t errori;
  char ultimoZero[9];
};

constexpr uint8_t MAX_EVENTI = 8;

void osserva(const link_antenna::Stato& s);  // dal loop()
uint8_t elenco(Evento* out, uint8_t max);    // dal più recente
Contatori contatori();
void oraTesto(char* out, unsigned n);

}  // namespace eventi
