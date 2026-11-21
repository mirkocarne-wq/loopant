// Pagina web del pannello (porta 80, rete di casa): stato, comandi all'antenna, vista e tocco dello schermo.
// Indirizzo: http://<IP del pannello>/ oppure http://loopant.local/
// ATTENZIONE: nessuna password. Chiunque sia sulla rete di casa può comandare l'antenna.
#pragma once
#include <stddef.h>

namespace server_web {

void inizia();  // avvia il task (core 0); il server parte quando il WiFi è collegato
bool attivo();
void indirizzo(char* out, size_t n);  // "http://192.168.1.26 · http://loopant.local" oppure ""

}  // namespace server_web
