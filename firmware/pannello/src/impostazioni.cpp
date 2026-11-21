#include "impostazioni.h"

#include <Preferences.h>
#include <string.h>

#include "config.h"

namespace impostazioni {
namespace {

constexpr uint8_t VERSIONE = 1;
Impostazioni imp;
Preferences nvs;

void predefinite() {
  memset(&imp, 0, sizeof(imp));
  imp.versione = VERSIONE;
  imp.tema = 0;
  imp.ultima40_hz = cfg::PROPOSTA_40M_HZ;
  imp.ultima20_hz = cfg::PROPOSTA_20M_HZ;
  imp.screensaver = 0;
  imp.attesa_min = 15;
  imp.luminosita = 70;
}

}  // namespace

void carica() {
  nvs.begin("pannello", false);
  if (nvs.getBytes("imp", &imp, sizeof(imp)) != sizeof(imp) || imp.versione != VERSIONE) {
    predefinite();
    salva();
  }
  imp.nominativo[sizeof(imp.nominativo) - 1] = 0;
  imp.locator[sizeof(imp.locator) - 1] = 0;
}

void salva() { nvs.putBytes("imp", &imp, sizeof(imp)); }

Impostazioni& get() { return imp; }

}  // namespace impostazioni
