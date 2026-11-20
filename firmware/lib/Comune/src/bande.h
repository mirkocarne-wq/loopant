// Bande gestite dall'antenna (limiti IARU Regione 1).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace bande {

enum class Banda : uint8_t { NESSUNA = 0, B40M = 40, B20M = 20 };

struct Info {
  Banda banda;
  uint32_t min_hz;
  uint32_t max_hz;
  uint32_t proposta_hz;  // frequenza proposta dal pulsante della banda
  const char* nome;
};

constexpr Info ELENCO[] = {
    {Banda::B40M, 7000000, 7200000, 7100000, "40 m"},
    {Banda::B20M, 14000000, 14350000, 14200000, "20 m"},
};
constexpr size_t NUMERO = sizeof(ELENCO) / sizeof(ELENCO[0]);

inline const Info* info(Banda b) {
  for (const Info& i : ELENCO)
    if (i.banda == b) return &i;
  return nullptr;
}

inline Banda di(uint32_t freq_hz) {
  for (const Info& i : ELENCO)
    if (freq_hz >= i.min_hz && freq_hz <= i.max_hz) return i.banda;
  return Banda::NESSUNA;
}

}  // namespace bande
