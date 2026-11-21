#include "motore_antenna.h"

#include <Preferences.h>

#include "link_antenna.h"

namespace motore_antenna {
namespace {

const uint16_t KBIT[] = {125, 250, 500, 1000};
Scelte s = {0, 1, 2};
Preferences nvs;

void salva() {
  nvs.putUChar("tipo", s.tipo);
  nvs.putUShort("canid", s.canId);
  nvs.putUChar("kbit", s.kbitIdx);
}

}  // namespace

void carica() {
  nvs.begin("motore", false);
  s.tipo = nvs.getUChar("tipo", 0) ? 1 : 0;
  s.canId = nvs.getUShort("canid", 1);
  s.kbitIdx = nvs.getUChar("kbit", 2);
  if (s.canId < 1 || s.canId > 2047) s.canId = 1;
  if (s.kbitIdx > 3) s.kbitIdx = 2;
}

Scelte& scelte() { return s; }

uint16_t kbit() { return KBIT[s.kbitIdx < 4 ? s.kbitIdx : 2]; }

void inviaTipo() {
  salva();
  link_antenna::invia(proto::Tipo::IMPOSTA, (uint32_t)proto::Parametro::TIPO_ANTENNA, s.tipo);
}

void inviaCan() {
  salva();
  // due comandi: link_antenna mette il secondo in coda finché l'antenna non ha risposto al primo
  link_antenna::invia(proto::Tipo::IMPOSTA, (uint32_t)proto::Parametro::CAN_ID, s.canId);
  link_antenna::invia(proto::Tipo::IMPOSTA, (uint32_t)proto::Parametro::CAN_KBIT, kbit());
}

}  // namespace motore_antenna
