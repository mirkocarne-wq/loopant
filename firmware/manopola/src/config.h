// Configurazione della manopola remota: pin della scheda Waveshare ESP32-S3-Knob-Touch-LCD-1.8 (dallo schema).
#pragma once
#include <stdint.h>

namespace cfg {

// Microfono MEMS PDM MSM261D4030H1CPM (canale sinistro)
constexpr int PIN_MIC_CLK = 45;
constexpr int PIN_MIC_DATI = 46;
constexpr uint32_t FREQ_AUDIO_HZ = 8000;  // ai decoder; il microfono lavora a 4 volte tanto

// Manopola (encoder dell'ESP32-S3)
constexpr int PIN_ENCODER_A = 8;
constexpr int PIN_ENCODER_B = 7;

// Collegamento con il pannello
constexpr uint8_t CANALE_WIFI = 1;  // predefinito; poi associazione e ricerca del canale (NVS)

}  // namespace cfg
