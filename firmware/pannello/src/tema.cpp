#include "tema.h"

namespace tema {
namespace {

Tema elenco[NUMERO];
uint8_t attuale = 0;

lv_color_t h(uint32_t c) { return lv_color_hex(c); }

// Segmento spento: il colore acceso mescolato al fondo (circa 9 %).
lv_color_t spento(lv_color_t acceso, lv_color_t fondo) { return lv_color_mix(acceso, fondo, 24); }

void completa(Tema& t) {
  t.segOffAccent = spento(t.accent, t.surface);
  t.segOffText = spento(t.text, t.surface);
}

void mappa(Tema& t, uint32_t mare, uint32_t terra, uint32_t costa, uint32_t linea, uint32_t sole, uint8_t notte) {
  t.mapSea = h(mare);
  t.mapLand = h(terra);
  t.mapCoast = h(costa);
  t.mapLine = h(linea);
  t.sun = h(sole);
  t.mapNotte = notte;
}

}  // namespace

void inizia() {
  elenco[0] = {"Notte", h(0x0E1217), h(0x171D24), h(0x212932), h(0x2D3742), h(0x3A4654),
               h(0xE8EDF2), h(0xA3AFBB),
               h(0xFFB547), h(0x1A1206), h(0x3A2C10),
               h(0x46C08A), h(0x12301F), h(0x6FD9A6),
               h(0xD13438), h(0xFFFFFF), h(0x3B1517), h(0xFF9A9C),
               h(0x5AA9F0), h(0x16304A), h(0x7DC0FF)};
  elenco[1] = {"Giorno", h(0xEEF1F4), h(0xFFFFFF), h(0xE3E8ED), h(0xC9D1DA), h(0x9AA6B2),
               h(0x12171D), h(0x4A5664),
               h(0xB35A00), h(0xFFFFFF), h(0xFBE9D4),
               h(0x1E7F4F), h(0xDDF3E7), h(0x17663F),
               h(0xC4302B), h(0xFFFFFF), h(0xFBE1E0), h(0xA12520),
               h(0x1F6FC5), h(0xDCEBFB), h(0x1A5DA6)};
  elenco[2] = {"Fosfori verdi", h(0x020A04), h(0x041409), h(0x072010), h(0x0F3D1C), h(0x1B6A31),
               h(0x9CFFA9), h(0x4FD068),
               h(0xC8FFD0), h(0x021006), h(0x0D3A1A),
               h(0x9CFFA9), h(0x0D3A1A), h(0x9CFFA9),
               h(0xC4302B), h(0xFFFFFF), h(0x3A0F0C), h(0xFF8A80),
               h(0x6BE57F), h(0x0D3A1A), h(0xC8FFD0)};
  elenco[3] = {"Fosfori ambra", h(0x0A0602), h(0x140C03), h(0x201406), h(0x4A2E0C), h(0x7A4C14),
               h(0xFFC46B), h(0xD08A2E),
               h(0xFFE0A8), h(0x140C03), h(0x3A2408),
               h(0xFFC46B), h(0x3A2408), h(0xFFC46B),
               h(0xC4302B), h(0xFFFFFF), h(0x3A0F0C), h(0xFF8A80),
               h(0xE8A245), h(0x3A2408), h(0xFFE0A8)};
  mappa(elenco[0], 0x0B1F33, 0x22384C, 0x3A5670, 0xFFB547, 0xFFD27A, 130);
  mappa(elenco[1], 0xCFE3F5, 0xB5CFAE, 0x7E9F76, 0xB35A00, 0xE08A00, 90);
  mappa(elenco[2], 0x020A04, 0x0A2A13, 0x4FD068, 0x9CFFA9, 0xC8FFD0, 150);
  mappa(elenco[3], 0x0A0602, 0x2E1C06, 0xD08A2E, 0xFFC46B, 0xFFE0A8, 150);
  for (Tema& t : elenco) completa(t);
}

const Tema& di(uint8_t indice) { return elenco[indice < NUMERO ? indice : 0]; }
const Tema& corrente() { return elenco[attuale]; }
uint8_t indiceCorrente() { return attuale; }
void imposta(uint8_t indice) { attuale = indice < NUMERO ? indice : 0; }

}  // namespace tema
