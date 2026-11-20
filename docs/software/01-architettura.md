# 01 — Architettura

```
[Waveshare ESP32-S3-Touch-LCD-7B, in casa]  <-- ESP-NOW -->  [ESP32 DevKit 38 pin, all'antenna]
  unità di CONTROLLO                                          unità ANTENNA
  tastierino, stato, LVGL                                     TMC2209 + motore, finecorsa, calibrazione
```

- L'**unità antenna** è autonoma: conosce la tabella di calibrazione, decide quando azzerare
  e come muovere il motore. Applica le sicurezze anche se il controllo sparisce (heartbeat).
- L'**unità di controllo** è solo interfaccia: invia la frequenza voluta e mostra lo stato.
- Entrambe hanno una **console seriale** per provare tutto anche senza lo schermo.

## Struttura del codice
```
firmware/
  lib/Comune/src/     codice condiviso dalle due unità
    protocollo.h        messaggi ESP-NOW
    bande.h             limiti delle bande 40 m / 20 m
    collegamento.*      ESP-NOW cifrato con un solo peer
    segreti.esempio.h   chiavi PMK/LMK di esempio (copiare in segreti.h, non versionato)
  antenna/            progetto PlatformIO dell'unità antenna (ESP32 DevKit)
    include/config.h    pin, meccanica, velocità, tempi
    src/movimento.*     motore, finecorsa, azzeramento, recupero del gioco
    src/calibrazione.*  tabella posizione ↔ frequenza in NVS
    src/main.cpp        gestione comandi, heartbeat, console seriale
  controllo/          pannello in casa (Waveshare 7B): da scrivere dopo i requisiti
```

## Toolchain
- Unità antenna: **PlatformIO** con framework **Arduino** (piattaforma `espressif32@6.9.0`, core 2.0.17).
  Librerie: FastAccelStepper 2.0.1 (impulsi STEP), TMCStepper 0.7.3 (UART del TMC2209).
- Pannello: core Arduino **3.x** + LVGL 8.4 (come gli esempi Waveshare); ambiente di sviluppo da scegliere.

## Build
```bash
cd firmware/antenna && pio run            # compila l'unità antenna (versione TX)
cd firmware/antenna && pio run -e rx      # compila l'unità antenna (versione RX, SERVO42D)
pio run -t upload && pio device monitor   # carica e apre la console seriale
```
