// Relè dei condensatori in serie al variabile (solo antenna RX): con C1, o C1 e C2, in serie la capacità
// totale scende sotto il minimo del variabile (circa 20 pF) e si arriva ai 17-10 m. Combinazione per banda:
// predefinita qui sotto (stime), modificabile dalla console con "serie" e salvata in NVS.
#pragma once
#include <Print.h>
#include <stdint.h>

#include "bande.h"

namespace rele {

constexpr uint8_t C1 = 1 << 0;  // K1 eccitato: C1 in serie
constexpr uint8_t C2 = 1 << 1;  // K2 eccitato: anche C2 in serie

void inizia(bool tx);  // TX: niente relè, pin non toccati
uint8_t perBanda(bande::Banda b);
bool imposta(bande::Banda b, uint8_t combinazione);  // salva in NVS (0-3)
// Mette i relè nella combinazione della banda; se cambiano aspetta che i contatti si fermino.
void scegli(bande::Banda b);
uint8_t attuale();
bool giusti(bande::Banda b);  // relè già nella combinazione della banda (sempre vero sulla TX)
const char* nome(uint8_t combinazione);  // "variabile", "C1", "C1+C2"
void stampa(Print& out);

}  // namespace rele
