// Tipo di antenna (TX o RX) e meccanica corrispondente. Firmware unico: il tipo si sceglie dal pannello
// (Impostazioni -> Antenna) o dalla console ("tipo tx|rx") e resta in NVS. Al cambio l'unità si riavvia:
// driver, CAN e calibrazione ripartono con il tipo nuovo e serve un nuovo azzeramento.
#pragma once
#include "config.h"

namespace tipo_antenna {

void inizia();  // da chiamare per primo in setup()
bool tx();
const cfg::Meccanica& meccanica();
void salva(bool tx);  // solo NVS: il chiamante poi riavvia

}  // namespace tipo_antenna
