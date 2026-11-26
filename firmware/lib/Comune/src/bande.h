// Bande gestite dall'antenna (limiti IARU Regione 1, da verificare sul band plan ufficiale e sulle norme italiane,
// soprattutto i 60 m). L'antenna RX accorda da 80 a 10 m (17-10 m con i condensatori in serie dei relè),
// la TX solo 40 e 20 m (condensatore a stantuffo, escursione circa 100 pF).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace bande {

// Il valore è la banda in metri: viaggia così nel protocollo (RispostaStato.banda, RispostaCal.banda).
enum class Banda : uint8_t {
  NESSUNA = 0,
  B80M = 80,
  B60M = 60,
  B40M = 40,
  B30M = 30,
  B20M = 20,
  B17M = 17,
  B15M = 15,
  B12M = 12,
  B10M = 10,
};

struct Info {
  Banda banda;
  uint32_t min_hz;
  uint32_t max_hz;
  uint32_t proposta_hz;  // frequenza proposta dal pulsante della banda
  const char* nome;
  bool tx;  // accordabile anche dall'antenna TX
};

// In ordine di frequenza crescente (la manopola passa alla banda successiva oltre il bordo alto).
constexpr Info ELENCO[] = {
    {Banda::B80M, 3500000, 3800000, 3700000, "80 m", false},
    {Banda::B60M, 5351500, 5366500, 5357000, "60 m", false},
    {Banda::B40M, 7000000, 7200000, 7100000, "40 m", true},
    {Banda::B30M, 10100000, 10150000, 10120000, "30 m", false},
    {Banda::B20M, 14000000, 14350000, 14200000, "20 m", true},
    {Banda::B17M, 18068000, 18168000, 18120000, "17 m", false},
    {Banda::B15M, 21000000, 21450000, 21200000, "15 m", false},
    {Banda::B12M, 24890000, 24990000, 24940000, "12 m", false},
    {Banda::B10M, 28000000, 29700000, 28500000, "10 m", false},
};
constexpr size_t NUMERO = sizeof(ELENCO) / sizeof(ELENCO[0]);

inline const Info* info(Banda b) {
  for (const Info& i : ELENCO)
    if (i.banda == b) return &i;
  return nullptr;
}

// Posizione nell'ELENCO, -1 se la banda non c'è
inline int indice(Banda b) {
  for (size_t i = 0; i < NUMERO; i++)
    if (ELENCO[i].banda == b) return (int)i;
  return -1;
}

inline Banda di(uint32_t freq_hz) {
  for (const Info& i : ELENCO)
    if (freq_hz >= i.min_hz && freq_hz <= i.max_hz) return i.banda;
  return Banda::NESSUNA;
}

// Bande del tipo di antenna: la TX solo quelle con tx = true
inline bool disponibile(Banda b, bool tx) {
  const Info* i = info(b);
  return i && (!tx || i->tx);
}

inline Banda di(uint32_t freq_hz, bool tx) {
  Banda b = di(freq_hz);
  return disponibile(b, tx) ? b : Banda::NESSUNA;
}

}  // namespace bande
