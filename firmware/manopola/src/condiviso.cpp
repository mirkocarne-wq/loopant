#include "condiviso.h"

#include <Arduino.h>

namespace condiviso {
namespace {
portMUX_TYPE blocco = portMUX_INITIALIZER_UNLOCKED;
Pannello pannello_ = {};
Comandi comandi_ = {false, 0, 7100000, 0, 0, 0};
}  // namespace

Pannello pannello() {
  portENTER_CRITICAL(&blocco);
  Pannello p = pannello_;
  portEXIT_CRITICAL(&blocco);
  return p;
}

void pannello(const Pannello& p) {
  portENTER_CRITICAL(&blocco);
  pannello_ = p;
  portEXIT_CRITICAL(&blocco);
}

Comandi comandi() {
  portENTER_CRITICAL(&blocco);
  Comandi c = comandi_;
  portEXIT_CRITICAL(&blocco);
  return c;
}

void comandi(const Comandi& c) {
  portENTER_CRITICAL(&blocco);
  comandi_ = c;
  portEXIT_CRITICAL(&blocco);
}

}  // namespace condiviso
