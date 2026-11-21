# Simulatore del pannello sul PC

Compila la **vera interfaccia** del pannello (`src/ui`, temi, mappa, display a 7 segmenti, font) con LVGL 8.4
su Linux e salva le schermate in PNG. Collegamento con l'antenna, WiFi, audio e decoder sono finti, con dati di esempio.

Serve per controllare l'impaginazione senza la scheda. Non sostituisce la prova sul 7B (driver, touch, prestazioni).

## Uso
Requisiti: gcc/g++, Python 3 con Pillow, la libreria lvgl 8.4 (quella installata da arduino-cli va bene).
```bash
firmware/pannello/simulatore/compila.sh ~/Arduino/libraries/lvgl
```
Le schermate finiscono in `firmware/pannello/simulatore/schermate/` (non versionate). Le ultime generate sono
copiate in [docs/software/schermate/](../../../docs/software/schermate/).

## File
- `simulatore.cpp`: schermo virtuale 1024×600, dati finti, sequenza delle schermate e dei temi.
- `stub/`: versioni finte di `Arduino.h`, `Preferences.h`, `esp_heap_caps.h`.
