// Decoder CW/RTTY della manopola: sorgente microfono o segnale di prova, impostazioni dal pannello,
// testo con contatore (il pannello prende i caratteri nuovi anche se perde qualche pacchetto).
#pragma once
#include <protocollo.h>
#include <stddef.h>
#include <stdint.h>

namespace decodifica {

void inizia();
void configura(const proto::ManopolaImposta& imp);  // decoder e sorgente (solo se cambiati)
void riempi(proto::ManopolaStato& s);               // campi del decoder e coda del testo
uint32_t testo(char* out, size_t n);                // ultimi caratteri (per il display); restituisce nTesto
float livello();                                    // livello audio 0-1
bool attivo();                                      // decoder acceso (stato del pannello)
bool rtty();
float wpm();
bool agganciato();
proto::SorgenteAudio sorgente();

}  // namespace decodifica
