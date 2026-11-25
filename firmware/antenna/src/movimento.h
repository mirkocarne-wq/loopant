// Movimento del condensatore: smista i comandi al driver del tipo di antenna
// (TX: STEP/DIR con il TMC2209; RX: CAN con il SERVO42D).
// Regole: docs/software/02-unita-antenna.md
#pragma once
#include <stdint.h>

#include "protocollo.h"

namespace movimento {

void inizia();
void aggiorna();  // da chiamare a ogni giro del loop

// Posizioni in passi con lo 0 dove finisce l'azzeramento (lì l'antenna resta fino al primo TUNE).
// GOTO e jog normale stanno sopra posMin (spazio per il recupero del gioco); il jog libero scende fino a 0.
// false se non azzerato. Con recupero (normale) arriva sempre dal verso crescente; senza, va diretto
// (ritocco fine della manopola: niente andata e ritorno, il prossimo GOTO riallinea).
bool vaiA(int32_t pos, bool recupero = true);
void azzera();
void ferma();  // con decelerazione

bool inMovimento();  // compreso l'azzeramento
bool inAzzeramento();
bool azzerato();
int32_t posizione();
bool finecorsaMin();
bool finecorsaMax();
void stampaIngressi();  // console "io": stato dei finecorsa (RX: letto dal SERVO42D, arriva con la risposta)
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
