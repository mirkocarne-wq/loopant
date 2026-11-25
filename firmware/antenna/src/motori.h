// I due driver del motore, con la stessa interfaccia:
// - stepdir: antenna TX, STEP/DIR/EN con il TMC2209, finecorsa sui GPIO dell'ESP32;
// - can42d: antenna RX, SERVO42D comandato via CAN, finecorsa collegati al SERVO42D (zero su En, limite CW su Dir).
// movimento.cpp usa quello del tipo di antenna.
#pragma once
#include <stdint.h>

#include "protocollo.h"

#define INTERFACCIA_MOTORE                       \
  void inizia();                                 \
  void aggiorna();                               \
  bool vaiA(int32_t pos, bool recupero);         \
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
void stampaIngressi();                       // legge i finecorsa (0x34) e li scrive sulla console
}  // namespace can42d
