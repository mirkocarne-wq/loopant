// Movimento del condensatore: smista i comandi al driver scelto (STEP/DIR o CAN).
// Regole: docs/software/02-unita-antenna.md
#pragma once
#include <stdint.h>

#include "protocollo.h"

namespace movimento {

enum class Modo : uint8_t { STEP_DIR = 0, CAN = 1 };

void inizia();
void aggiorna();  // da chiamare a ogni giro del loop

bool vaiA(int32_t pos);  // false se non azzerato; arriva sempre dal verso crescente
void azzera();
void ferma();  // con decelerazione

bool inMovimento();  // compreso l'azzeramento
bool inAzzeramento();
bool azzerato();
int32_t posizione();
bool finecorsaMin();
bool finecorsaMax();
bool driverAbilitato();

proto::Errore errore();
void cancellaErrore();

// Driver del motore (solo antenna RX: il TX usa sempre STEP/DIR). Salvato in NVS.
// false se non si può cambiare (antenna TX o motore in movimento). Dopo il cambio serve un nuovo azzeramento.
bool impostaModo(Modo m);
Modo modo();
bool impostaCan(uint16_t id, uint16_t kbit);  // indirizzo 1-2047, velocità 125/250/500/1000
uint16_t canId();
uint16_t canKbit();

}  // namespace movimento
