// Scheda del pannello: Waveshare ESP32-S3-Touch-LCD-7B (predefinita) oppure ESP32-S3-Touch-LCD-5B
// (5", 1024x600: compila.sh 5b, che definisce PANNELLO_5B=1). Stessa risoluzione e stessi pin RGB; sul 5B cambiano
// le temporizzazioni dello schermo e l'IO expander (CH422G: niente PWM, retroilluminazione solo accesa/spenta).
#pragma once
#ifndef PANNELLO_5B
#define PANNELLO_5B 0
#endif
