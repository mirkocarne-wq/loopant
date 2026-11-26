#include "memorie.h"

#include <Preferences.h>
#include <bande.h>
#include <string.h>

namespace memorie {
namespace {

// Frequenze d'uso comune (FT8/FT4/WSPR e centri di attività QRP): da verificare sul band plan IARU R1.
// Quelle delle bande che l'antenna associata non accorda non compaiono (la TX solo 40 e 20 m).
const Memoria PREDEFINITE[] = {
    {"FT8", Modo::FT8, 3573000},      {"FT4", Modo::FT4, 3575500},     {"WSPR", Modo::WSPR, 3568600},
    {"QRP CW", Modo::CW, 3560000},    {"QRP SSB", Modo::SSB, 3690000},
    {"FT8", Modo::FT8, 5357000},      {"WSPR", Modo::WSPR, 5364700},
    {"FT8", Modo::FT8, 7074000},      {"FT4", Modo::FT4, 7047500},     {"WSPR", Modo::WSPR, 7038600},
    {"QRP CW", Modo::CW, 7030000},    {"QRP SSB", Modo::SSB, 7090000},
    {"FT8", Modo::FT8, 10136000},     {"FT4", Modo::FT4, 10140000},    {"WSPR", Modo::WSPR, 10138700},
    {"QRP CW", Modo::CW, 10116000},
    {"FT8", Modo::FT8, 14074000},     {"FT4", Modo::FT4, 14080000},    {"WSPR", Modo::WSPR, 14095600},
    {"QRP CW", Modo::CW, 14060000},   {"QRP SSB", Modo::SSB, 14285000},
    {"FT8", Modo::FT8, 18100000},     {"FT4", Modo::FT4, 18104000},    {"WSPR", Modo::WSPR, 18104600},
    {"QRP CW", Modo::CW, 18086000},   {"QRP SSB", Modo::SSB, 18130000},
    {"FT8", Modo::FT8, 21074000},     {"FT4", Modo::FT4, 21140000},    {"WSPR", Modo::WSPR, 21094600},
    {"QRP CW", Modo::CW, 21060000},   {"QRP SSB", Modo::SSB, 21285000},
    {"FT8", Modo::FT8, 24915000},     {"FT4", Modo::FT4, 24919000},    {"WSPR", Modo::WSPR, 24924600},
    {"QRP CW", Modo::CW, 24906000},   {"QRP SSB", Modo::SSB, 24950000},
    {"FT8", Modo::FT8, 28074000},     {"FT4", Modo::FT4, 28180000},    {"WSPR", Modo::WSPR, 28124600},
    {"QRP CW", Modo::CW, 28060000},   {"QRP SSB", Modo::SSB, 28360000},
};
constexpr uint8_t N_PREDEFINITE = sizeof(PREDEFINITE) / sizeof(PREDEFINITE[0]);

struct Archivio {
  uint8_t versione;
  uint8_t n;
  Memoria m[MAX_PERSONALI];
};

Archivio arch = {};
Preferences nvs;

void scrivi() { nvs.putBytes("mem", &arch, sizeof(arch)); }

}  // namespace

void carica() {
  nvs.begin("memorie", false);
  if (nvs.getBytes("mem", &arch, sizeof(arch)) != sizeof(arch) || arch.versione != 1 || arch.n > MAX_PERSONALI) {
    memset(&arch, 0, sizeof(arch));
    arch.versione = 1;
  }
}

uint8_t numeroPredefinite() { return N_PREDEFINITE; }
const Memoria& predefinita(uint8_t i) { return PREDEFINITE[i < N_PREDEFINITE ? i : 0]; }
uint8_t numeroPersonali() { return arch.n; }
const Memoria& personale(uint8_t i) { return arch.m[i < arch.n ? i : 0]; }

bool aggiungi(const char* nome, uint32_t freq_hz) {
  if (arch.n >= MAX_PERSONALI || bande::di(freq_hz) == bande::Banda::NESSUNA) return false;
  Memoria& m = arch.m[arch.n++];
  memset(&m, 0, sizeof(m));
  strncpy(m.nome, nome && *nome ? nome : "Memoria", sizeof(m.nome) - 1);
  m.modo = Modo::MIA;
  m.freq_hz = freq_hz;
  scrivi();
  return true;
}

void elimina(uint8_t i) {
  if (i >= arch.n) return;
  for (uint8_t k = i; k + 1 < arch.n; k++) arch.m[k] = arch.m[k + 1];
  arch.n--;
  scrivi();
}

const char* nomeModo(Modo m) {
  switch (m) {
    case Modo::FT8: return "FT8";
    case Modo::FT4: return "FT4";
    case Modo::WSPR: return "WSPR";
    case Modo::CW: return "CW";
    case Modo::SSB: return "SSB";
    case Modo::MIA: return "MIA";
  }
  return "";
}

}  // namespace memorie
