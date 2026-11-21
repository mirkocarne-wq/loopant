// Screensaver (requisiti V1-V4): Orologio, Frequenza, Grey line o schermo spento.
// Parte dopo l'attesa impostata senza tocchi; un tocco lo chiude.
#pragma once
#include "../link_antenna.h"

namespace screensaver {

void aggiorna(const link_antenna::Stato& s);  // dal timer dell'interfaccia
void chiudi();
bool attivo();
void anteprima();  // mostra subito lo screensaver scelto (da Impostazioni)

}  // namespace screensaver
