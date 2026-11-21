#include "movimento.h"

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"
#include "motori.h"
#include "tipo_antenna.h"

namespace movimento {
namespace {

uint16_t id_ = cfg::CAN_ID_PREDEFINITO;
uint16_t kbit_ = cfg::CAN_KBIT_PREDEFINITO;
Preferences nvs;

bool can() { return !tipo_antenna::tx(); }  // TX: STEP/DIR (TMC2209); RX: CAN (SERVO42D)

// I driver contano dal finecorsa; fuori da qui lo 0 è dove finisce l'azzeramento: sul finecorsa con il SERVO42D
// via CAN (azzeramento fatto dal servo), allo stacco con STEP/DIR. Lì l'antenna resta fino al primo TUNE.
int32_t origine() { return can() ? 0 : tipo_antenna::meccanica().staccoPassi; }

bool kbitValido(uint16_t k) { return k == 125 || k == 250 || k == 500 || k == 1000; }

void salva() {
  nvs.putUShort("canid", id_);
  nvs.putUShort("cankbit", kbit_);
}

void avviaCan() {
  can42d::configura(id_, kbit_);
  can42d::inizia();
}

}  // namespace

void inizia() {
  nvs.begin("motore", false);
  id_ = nvs.getUShort("canid", cfg::CAN_ID_PREDEFINITO);
  kbit_ = nvs.getUShort("cankbit", cfg::CAN_KBIT_PREDEFINITO);
  if (!kbitValido(kbit_)) kbit_ = cfg::CAN_KBIT_PREDEFINITO;

  if (can()) avviaCan();
  else stepdir::inizia();
}

void aggiorna() {
  if (can()) can42d::aggiorna();
  else stepdir::aggiorna();
}

bool vaiA(int32_t pos, bool recupero) {
  pos += origine();
  return can() ? can42d::vaiA(pos, recupero) : stepdir::vaiA(pos, recupero);
}
void azzera() { can() ? can42d::azzera() : stepdir::azzera(); }
void ferma() { can() ? can42d::ferma() : stepdir::ferma(); }
bool inMovimento() { return can() ? can42d::inMovimento() : stepdir::inMovimento(); }
bool inAzzeramento() { return can() ? can42d::inAzzeramento() : stepdir::inAzzeramento(); }
bool azzerato() { return can() ? can42d::azzerato() : stepdir::azzerato(); }
int32_t posizione() { return (can() ? can42d::posizione() : stepdir::posizione()) - origine(); }
bool finecorsaMin() { return can() ? can42d::finecorsaMin() : stepdir::finecorsaMin(); }
bool finecorsaMax() { return can() ? can42d::finecorsaMax() : stepdir::finecorsaMax(); }
bool driverAbilitato() { return can() ? can42d::driverAbilitato() : stepdir::driverAbilitato(); }
proto::Errore errore() { return can() ? can42d::errore() : stepdir::errore(); }
void cancellaErrore() { can() ? can42d::cancellaErrore() : stepdir::cancellaErrore(); }

bool viaCan() { return can(); }

bool impostaCan(uint16_t id, uint16_t kbit) {
  if (tipo_antenna::tx() || id < 1 || id > 2047 || !kbitValido(kbit) || inMovimento()) return false;
  id_ = id;
  kbit_ = kbit;
  salva();
  if (can()) {  // riparte con i nuovi parametri
    can42d::fine();
    avviaCan();
  }
  return true;
}

uint16_t canId() { return id_; }
uint16_t canKbit() { return kbit_; }

}  // namespace movimento
