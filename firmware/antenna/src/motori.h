// I due driver del motore, con la stessa interfaccia:
// - stepdir: STEP/DIR/EN (TMC2209 sul TX, SERVO42D in modo a impulsi sull'RX), finecorsa sui GPIO;
// - can42d: SERVO42D comandato via CAN (solo RX), finecorsa collegati al SERVO42D.
// movimento.cpp sceglie quale usare.
#pragma once
#include <stdint.h>

#include "protocollo.h"

#define INTERFACCIA_MOTORE                       \
  void inizia();                                 \
  void aggiorna();                               \
  bool vaiA(int32_t pos);                        \
  void azzera();                                 \
  void ferma();                                  \
  bool inMovimento();                            \
  bool inAzzeramento();                          \
  bool azzerato();                               \
  int32_t posizione();                           \
  bool finecorsaMin();                           \
  bool finecorsaMax();                           \
  bool driverAbilitato();                        \
  proto::Errore errore();                        \
  void cancellaErrore();

namespace stepdir {
INTERFACCIA_MOTORE
}  // namespace stepdir

namespace can42d {
INTERFACCIA_MOTORE
void configura(uint16_t id, uint16_t kbit);  // da chiamare prima di inizia()
void fine();                                 // libera il bus (passaggio a STEP/DIR)
}  // namespace can42d
