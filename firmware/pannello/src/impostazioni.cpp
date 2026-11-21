#include "impostazioni.h"

#include <Preferences.h>
#include <string.h>

#include "config.h"

namespace impostazioni {
namespace {

// Versione 2: schermo, ora, rete e decoder. Con una versione diversa si torna ai valori predefiniti.
constexpr uint8_t VERSIONE = 2;
Impostazioni imp;
Preferences nvs;

// Credenziali QRZ in un blocco a parte: aggiungerle non azzera le altre impostazioni
constexpr uint8_t VERSIONE_QRZ = 1;
ImpostazioniQrz credQrz;

void predefinite() {
  memset(&imp, 0, sizeof(imp));
  imp.versione = VERSIONE;
  imp.tema = 0;
  imp.ultima40_hz = cfg::PROPOSTA_40M_HZ;
  imp.ultima20_hz = cfg::PROPOSTA_20M_HZ;
  imp.screensaver = 0;
  imp.attesa_min = 15;
  imp.luminosita = 80;
  imp.modoOra = 0;
  imp.fuso = 1;
  imp.decModo = 0;
  imp.cwTono = 700;
  imp.cwFiltro = 250;
  imp.cwWpm = 0;
  imp.rttyBaud = 0;
  imp.rttyShift = 170;
  imp.rttyMark = 2125;
  imp.rttyStop = 1;
  imp.rttyOpzioni = RTTY_UOS | RTTY_AFC;
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
  imp.wifiSsid[sizeof(imp.wifiSsid) - 1] = 0;
  imp.wifiPassword[sizeof(imp.wifiPassword) - 1] = 0;

  if (nvs.getBytes("qrz", &credQrz, sizeof(credQrz)) != sizeof(credQrz) || credQrz.versione != VERSIONE_QRZ) {
    memset(&credQrz, 0, sizeof(credQrz));
    credQrz.versione = VERSIONE_QRZ;
  }
  credQrz.utente[sizeof(credQrz.utente) - 1] = 0;
  credQrz.password[sizeof(credQrz.password) - 1] = 0;
  credQrz.chiave[sizeof(credQrz.chiave) - 1] = 0;
}

void salva() { nvs.putBytes("imp", &imp, sizeof(imp)); }

Impostazioni& get() { return imp; }

ImpostazioniQrz& qrz() { return credQrz; }

void salvaQrz() { nvs.putBytes("qrz", &credQrz, sizeof(credQrz)); }

}  // namespace impostazioni
