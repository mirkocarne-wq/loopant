#include "rele.h"

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"

namespace rele {
namespace {

bool presenti = false;  // solo antenna RX
uint8_t stato = 0;
Preferences nvs;

// Stime del 26/11/2026 con L circa 2,6-2,8 µH, variabile 20-1000 pF, 2 pF di parassite, C1 circa 33 pF e
// C2 circa 27 pF (docs/tecnico/03-rx-condensatore-meccanica.md): da verificare con il NanoVNA.
uint8_t predefinita(bande::Banda b) {
  switch (b) {
    case bande::Banda::B17M:
    case bande::Banda::B15M: return C1;
    case bande::Banda::B12M:
    case bande::Banda::B10M: return C1 | C2;
    default: return 0;
  }
}

void chiave(bande::Banda b, char out[12]) { snprintf(out, 12, "serie%u", (unsigned)b); }

void scrivi(uint8_t m) {
  digitalWrite(cfg::PIN_RELE_C1, (m & C1) ? HIGH : LOW);
  digitalWrite(cfg::PIN_RELE_C2, (m & C2) ? HIGH : LOW);
  stato = m;
}

}  // namespace

void inizia(bool tx) {
  presenti = !tx;
  if (!presenti) return;
  nvs.begin("rele", false);
  pinMode(cfg::PIN_RELE_C1, OUTPUT);
  pinMode(cfg::PIN_RELE_C2, OUTPUT);
  scrivi(0);
}

uint8_t perBanda(bande::Banda b) {
  if (!presenti || bande::indice(b) < 0) return 0;
  char k[12];
  chiave(b, k);
  uint8_t m = nvs.getUChar(k, 0xFF);
  return m <= (C1 | C2) ? m : predefinita(b);
}

bool imposta(bande::Banda b, uint8_t combinazione) {
  if (!presenti || bande::indice(b) < 0 || combinazione > (C1 | C2)) return false;
  char k[12];
  chiave(b, k);
  nvs.putUChar(k, combinazione);
  return true;
}

void scegli(bande::Banda b) {
  if (!presenti || bande::indice(b) < 0) return;
  uint8_t m = perBanda(b);
  if (m == stato) return;
  scrivi(m);
  delay(cfg::RELE_ASSESTAMENTO_MS);
}

uint8_t attuale() { return stato; }

bool giusti(bande::Banda b) { return !presenti || perBanda(b) == stato; }

const char* nome(uint8_t combinazione) {
  switch (combinazione) {
    case C1: return "C1 in serie";
    case C2: return "C2 in serie";
    case C1 | C2: return "C1+C2 in serie";
    default: return "variabile da solo";
  }
}

void stampa(Print& out) {
  if (!presenti) {
    out.println("Relè dei condensatori in serie: solo antenna RX");
    return;
  }
  out.printf("Relè ora: %s (K1 %s, K2 %s)\n", nome(stato), (stato & C1) ? "eccitato" : "a riposo",
             (stato & C2) ? "eccitato" : "a riposo");
  for (const bande::Info& i : bande::ELENCO)
    out.printf("  %-5s %s\n", i.nome, nome(perBanda(i.banda)));
}

}  // namespace rele
