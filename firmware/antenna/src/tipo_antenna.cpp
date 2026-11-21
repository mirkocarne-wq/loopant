#include "tipo_antenna.h"

#include <Preferences.h>

namespace tipo_antenna {
namespace {
bool tx_ = cfg::TIPO_TX_PREDEFINITO;
}  // namespace

void inizia() {
  Preferences nvs;
  nvs.begin("loopant", true);
  tx_ = nvs.getBool("tipotx", cfg::TIPO_TX_PREDEFINITO);
  nvs.end();
}

bool tx() { return tx_; }

const cfg::Meccanica& meccanica() { return tx_ ? cfg::TX : cfg::RX; }

void salva(bool tx) {
  Preferences nvs;
  nvs.begin("loopant", false);
  nvs.putBool("tipotx", tx);
  nvs.end();
}

}  // namespace tipo_antenna
