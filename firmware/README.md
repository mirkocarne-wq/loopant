# Firmware

| Cartella | Contenuto | Stato |
|---|---|---|
| `lib/Comune/` | Protocollo ESP-NOW, bande, collegamento cifrato, chiavi | scritto |
| `antenna/` | Unità antenna (ESP32 DevKit 38 pin), ambienti `tx` e `rx` | scritto, compila; da provare sul banco |
| `pannello/` | Pannello in casa (Waveshare ESP32-S3-Touch-LCD-7B), Arduino core 3.1.1 + LVGL 8.4 | prima versione, compila; da provare sulla scheda |

Documentazione: [`docs/software/`](../docs/software/README.md).

```bash
cd antenna && pio run -e tx                          # compila la versione TX
cd antenna && pio run -e tx -t upload -t monitor     # carica e apre la console
```

Prima dell'uso reale copiare `lib/Comune/src/segreti.esempio.h` in `lib/Comune/src/segreti.h`
e cambiare le chiavi (uguali sulle due unità).
