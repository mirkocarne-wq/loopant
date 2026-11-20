// Movimento del condensatore: motore, finecorsa, azzeramento e recupero del gioco.
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

proto::Errore errore();
void cancellaErrore();

}  // namespace movimento
