// Copia su microSD (slot del pannello): impostazioni, ultime frequenze, memorie personali e tabelle di
// calibrazione lette dall'antenna, in un file di testo (/loopant/copia.txt, più una copia con data e ora).
// Il ripristino rimette tutto sul pannello, riscrive le tabelle sull'antenna (CAL_SCRIVI), le rilegge per
// controllo e riavvia il pannello. Le password (WiFi, QRZ) non vengono scritte sulla scheda.
// Le richieste arrivano da LVGL o dalla console; il lavoro lo fa aggiorna(), dal loop().
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace copia_sd {

enum class Fase : uint8_t { PRONTA, IN_CORSO, FINITA, ERRORE };

struct Stato {
  Fase fase;
  char testo[160];
};

bool salva();       // false se un'operazione è già in corso
bool ripristina();  // false se un'operazione è già in corso
void aggiorna();
Stato stato();

}  // namespace copia_sd
