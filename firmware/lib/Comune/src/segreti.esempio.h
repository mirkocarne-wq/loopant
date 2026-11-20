// Chiavi ESP-NOW DI ESEMPIO: non sicure.
// Copiare questo file in segreti.h (non versionato, vedi .gitignore) e cambiare i byte.
// Le due unità devono avere le STESSE chiavi.
#pragma once
#include <stdint.h>

namespace segreti {
constexpr uint8_t PMK[16] = {'l', 'o', 'o', 'p', 'a', 'n', 't', '-', 'p', 'm', 'k', '-', '0', '0', '0', '1'};
constexpr uint8_t LMK[16] = {'l', 'o', 'o', 'p', 'a', 'n', 't', '-', 'l', 'm', 'k', '-', '0', '0', '0', '1'};
}  // namespace segreti
