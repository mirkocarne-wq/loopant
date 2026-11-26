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

// Ultime frequenze per banda (indice dell'ELENCO) in un blocco a parte, come le credenziali QRZ
struct Ultime {
  uint8_t versione;
  uint8_t numero;  // bande::NUMERO quando è stato scritto
  uint8_t antennaTx;
  uint32_t usata;
  uint32_t hz[bande::NUMERO];
};
constexpr uint8_t VERSIONE_ULTIME = 1;
Ultime ultime;

void ultimePredefinite() {
  memset(&ultime, 0, sizeof(ultime));
  ultime.versione = VERSIONE_ULTIME;
  ultime.numero = bande::NUMERO;
  for (size_t i = 0; i < bande::NUMERO; i++) ultime.hz[i] = bande::ELENCO[i].proposta_hz;
  // dalle impostazioni di prima (solo 40 e 20 m)
  int i40 = bande::indice(bande::Banda::B40M), i20 = bande::indice(bande::Banda::B20M);
  if (bande::di(imp.ultima40_hz) == bande::Banda::B40M) ultime.hz[i40] = imp.ultima40_hz;
  if (bande::di(imp.ultima20_hz) == bande::Banda::B20M) ultime.hz[i20] = imp.ultima20_hz;
  ultime.usata = ultime.hz[i40];
}

void salvaUltime() { nvs.putBytes("ultime", &ultime, sizeof(ultime)); }

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

  if (nvs.getBytes("ultime", &ultime, sizeof(ultime)) != sizeof(ultime) || ultime.versione != VERSIONE_ULTIME ||
      ultime.numero != bande::NUMERO)
    ultimePredefinite();
  for (size_t i = 0; i < bande::NUMERO; i++)
    if (bande::di(ultime.hz[i]) != bande::ELENCO[i].banda) ultime.hz[i] = bande::ELENCO[i].proposta_hz;
  if (bande::di(ultime.usata) == bande::Banda::NESSUNA) ultime.usata = bande::ELENCO[0].proposta_hz;
}

void salva() { nvs.putBytes("imp", &imp, sizeof(imp)); }

Impostazioni& get() { return imp; }

ImpostazioniQrz& qrz() { return credQrz; }

void salvaQrz() { nvs.putBytes("qrz", &credQrz, sizeof(credQrz)); }

uint32_t ultima(bande::Banda b) {
  int i = bande::indice(b);
  return i < 0 ? 0 : ultime.hz[i];
}

uint32_t ultimaUsata() { return ultime.usata; }

void ricorda(uint32_t hz) {
  int i = bande::indice(bande::di(hz));
  if (i < 0) return;
  if (ultime.hz[i] == hz && ultime.usata == hz) return;
  ultime.hz[i] = hz;
  ultime.usata = hz;
  salvaUltime();
}

bool antennaTx() { return ultime.antennaTx; }

bool ricordaTipo(bool tx) {
  if ((bool)ultime.antennaTx == tx) return false;
  ultime.antennaTx = tx;
  salvaUltime();
  return true;
}

}  // namespace impostazioni
