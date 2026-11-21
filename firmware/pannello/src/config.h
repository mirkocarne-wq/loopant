// Configurazione del pannello in casa (Waveshare ESP32-S3-Touch-LCD-7B).
#pragma once
#include <stdint.h>

namespace cfg {

constexpr int16_t LARGHEZZA = 1024;
constexpr int16_t ALTEZZA = 600;
constexpr int16_t ALTEZZA_BARRA = 64;   // barra in alto
constexpr int16_t ALTEZZA_SCHEDE = 72;  // schede in basso

// Collegamento con l'antenna (docs/software/04-protocollo-espnow.md)
constexpr uint8_t CANALE_WIFI = 1;  // uguale a quello dell'antenna
constexpr uint32_t HEARTBEAT_MS = 1000;
constexpr uint32_t TIMEOUT_COLLEGAMENTO_MS = 3000;  // senza risposte: collegamento perso
constexpr uint32_t RIPETIZIONE_MS = 300;            // attesa prima di ripetere un comando
constexpr uint8_t MAX_TENTATIVI = 3;

// Frequenze proposte dai pulsanti di banda al primo avvio (poi: ultima usata)
constexpr uint32_t PROPOSTA_40M_HZ = 7100000;
constexpr uint32_t PROPOSTA_20M_HZ = 14200000;

}  // namespace cfg
