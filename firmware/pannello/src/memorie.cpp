#include "memorie.h"

#include <Preferences.h>
#include <bande.h>
#include <string.h>

namespace memorie {
namespace {

const Memoria PREDEFINITE[] = {
    {"FT8", Modo::FT8, 7074000},      {"FT4", Modo::FT4, 7047500},     {"WSPR", Modo::WSPR, 7038600},
    {"QRP CW", Modo::CW, 7030000},    {"QRP SSB", Modo::SSB, 7090000}, {"FT8", Modo::FT8, 14074000},
    {"FT4", Modo::FT4, 14080000},     {"WSPR", Modo::WSPR, 14095600},  {"QRP CW", Modo::CW, 14060000},
    {"QRP SSB", Modo::SSB, 14285000},
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
