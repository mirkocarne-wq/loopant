// Ricerca del canale del pannello per un'unità remota (antenna, nodo audio): il pannello segue il canale del
// router, che può cambiare. Si prova ogni canale, partendo da quello salvato, con un pacchetto che la radio del
// pannello conferma (ACK di ESP-NOW) solo sul canale giusto. Con il pannello acceso dura meno di un secondo.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "collegamento.h"

namespace ricerca_canale {

// true: pannello trovato, 'canale' è quello giusto e la radio resta lì; false: la radio torna su 'canale'.
// Blocca il chiamante (al massimo 13 canali x 2 tentativi x ackMs).
bool cerca(Collegamento& radio, uint8_t& canale, const void* pacchetto, size_t len, uint32_t ackMs);

}  // namespace ricerca_canale
