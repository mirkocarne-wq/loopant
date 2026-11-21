# Firmware

| Cartella | Contenuto | Stato |
|---|---|---|
| `lib/Comune/` | Protocollo ESP-NOW, bande, collegamento cifrato, chiavi | scritto |
| `antenna/` | Unità antenna (ESP32 DevKit 30 pin, WROOM-32), firmware unico per TX e RX, arduino-cli + core 3.1.1 | collegamento, display, associazione e CAN provati; motore da provare sul banco |
| `manopola/` | Manopola remota (Waveshare ESP32-S3-Knob-Touch-LCD-1.8): decoder dal microfono, ritocco fine, TUNE | associazione e decoder provati; JOG e TUNE da provare con l'antenna |
| `lib/Decoder/`, `lib/Grafica/` | Decoder CW/RTTY e decimatore; font e cifre a 7 segmenti | usate da pannello e manopola |
| `pannello/` | Pannello in casa (Waveshare ESP32-S3-Touch-LCD-7B), Arduino core 3.1.1 + LVGL 8.4 | provato sulla scheda con l'antenna |

Documentazione: [`docs/software/`](../docs/software/README.md).

```bash
antenna/compila.sh                      # compila l'unità antenna (tipo TX o RX scelto dal pannello)
antenna/compila.sh carica <porta>       # compila e carica
pannello/compila.sh carica <porta>      # pannello
```

Prima dell'uso reale copiare `lib/Comune/src/segreti.esempio.h` in `lib/Comune/src/segreti.h`
e cambiare le chiavi (uguali sulle due unità).
