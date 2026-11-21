// Ora del pannello: UTC e locale (fuso dalle impostazioni). Fonte: NTP via WiFi o console.
#pragma once
#include <stddef.h>
#include <time.h>

namespace ora {

void applicaFuso();  // da chiamare all'avvio e quando cambia il fuso
bool valida();       // l'ora è stata impostata (NTP o a mano)
void hhmmUtc(char* out, size_t n);
void hhmmLocale(char* out, size_t n);
void dataLocale(char* out, size_t n);  // "venerdì 21 novembre"

// Imposta l'ora UTC (anno, mese 1-12, giorno, ore, minuti).
bool impostaUtc(int anno, int mese, int giorno, int ore, int minuti);

}  // namespace ora
