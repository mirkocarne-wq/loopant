// Movimento del condensatore: smista i comandi al driver del tipo di antenna
// (TX: STEP/DIR con il TMC2209; RX: CAN con il SERVO42D).
// Regole: docs/software/02-unita-antenna.md
#pragma once
#include <stdint.h>

#include "protocollo.h"

namespace movimento {

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

// Il driver segue il tipo di antenna (tipo_antenna.h): true con l'antenna RX (SERVO42D via CAN).
bool viaCan();
// Parametri del bus CAN (solo antenna RX), salvati in NVS. false se non validi o con il motore in movimento.
bool impostaCan(uint16_t id, uint16_t kbit);  // indirizzo 1-2047, velocità 125/250/500/1000
uint16_t canId();
uint16_t canKbit();

}  // namespace movimento
