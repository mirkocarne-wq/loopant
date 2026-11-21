// Memorie di frequenza: predefinite (fisse) e personali (in NVS).
// Le frequenze predefinite sono da verificare sul band plan IARU Regione 1.
#pragma once
#include <stdint.h>

namespace memorie {

enum class Modo : uint8_t { FT8, FT4, WSPR, CW, SSB, MIA };

struct Memoria {
  char nome[20];
  Modo modo;
  uint32_t freq_hz;
};

constexpr uint8_t MAX_PERSONALI = 12;

void carica();
uint8_t numeroPredefinite();
const Memoria& predefinita(uint8_t i);
uint8_t numeroPersonali();
const Memoria& personale(uint8_t i);
bool aggiungi(const char* nome, uint32_t freq_hz);  // false se piena o frequenza fuori banda
void elimina(uint8_t i);
const char* nomeModo(Modo m);

}  // namespace memorie
