# loopant — Mag Loop automatica 40 m / 20 m

Progetto personale. Antenna magnetic loop per **40 m e 20 m**, da balcone (circa 4 m), con **sintonia automatica**:
sul pannello touch in casa si digita la frequenza e l'antenna va in risonanza.

Due versioni:
1. **RX** (solo ricezione): anello 1 m, variabile a lamelle 1000 pF, NEMA17 + MKS SERVO42D + riduttore 20:1.
2. **TX 100 W**: anello 1,5 m in tubo da 22 mm, condensatore a stantuffo (tubo in tubo con PTFE),
   attuatore lineare NEMA11 (vite T6×1, corsa 150 mm) pilotato da un TMC2209.

## Lingua e stile
- Si lavora **in italiano**: documentazione, commenti, messaggi di commit e interfaccia utente.
- Le stime di calcolo vanno indicate come **stime da verificare** con misure reali (NanoVNA).
  Non presentare come certo un dato non misurato; dove manca, scrivere "da verificare".
- Nei requisiti distinguere ciò che è già deciso da ciò che è una proposta da confermare.

## Struttura del repository
```
docs/tecnico/      progetto tecnico: anello, condensatori, meccanica, elettronica, alimentazione, acquisti
docs/software/     software: architettura, unità antenna, pannello (requisiti), protocollo, tuning, console
docs/memoria/      decisioni prese/scartate e punti aperti
docs/archivio/     documento originale del 20/11/2026 (non aggiornato)
firmware/lib/Comune/   codice condiviso: protocollo ESP-NOW, bande, collegamento, chiavi
firmware/antenna/      firmware unità antenna (ESP32 DevKit 38 pin), PlatformIO, ambienti tx e rx
firmware/pannello/     firmware pannello in casa (Waveshare 7B), Arduino core 3.1.1 + LVGL 8.4
```
Il firmware del **pannello in casa** (Waveshare ESP32-S3-Touch-LCD-7B) è alla prima versione (stato in
`firmware/pannello/README.md`). Requisiti e mockup approvati: `docs/software/03-unita-controllo.md`
(schermate, temi, screensaver), `08-decoder.md` (CW/RTTY) e `09-propagazione.md` (grey line, indici solari).
Le stringhe dell'interfaccia del pannello per ora devono essere **solo ASCII** (font Montserrat integrati in LVGL).

## Build
```bash
cd firmware/antenna && pio run -e tx   # versione TX (NEMA11 + TMC2209)
cd firmware/antenna && pio run -e rx   # versione RX (SERVO42D in STEP/DIR)
firmware/pannello/compila.sh           # pannello, con arduino-cli (core esp32 3.1.1, lvgl 8.4.0)
```
Le chiavi ESP-NOW reali vanno in `firmware/lib/Comune/src/segreti.h` (non versionato);
senza, si compila con le chiavi di esempio e un avviso.
Il pannello 7B usa il core Arduino ESP32 **3.1.1** (gli esempi Waveshare usano `esp_lcd` e `i2c_master` di IDF 5);
l'antenna resta su PlatformIO `espressif32@6.9.0` (core 2.0.17). Non usare `Wire` sul pannello: l'I2C è del driver Waveshare.

## Regole che il firmware deve rispettare
- **Avvicinarsi alla posizione target sempre dallo stesso verso**: il gioco del riduttore
  (30 arc-min ≈ 2,7 pF) diventa un offset costante assorbito dalla tabella di calibrazione.
- **Zero sui finecorsa a ogni cambio banda** (il TX è in ciclo aperto, senza encoder).
- **Driver disabilitato dopo ogni movimento** (motore libero, niente rumore HF durante l'ascolto).
- **Heartbeat ESP-NOW ogni secondo**; al timeout: motore fermo e driver disabilitato.
- In TX **non muovere il condensatore con RF ad alta potenza**: ricerca del ROS minimo a circa 5 W.
- ESP32 antenna: ingressi analogici solo su **ADC1 (GPIO 32–39)**, perché l'ADC2 non funziona con WiFi/ESP-NOW;
  evitare i pin di strapping **0, 2, 12, 15**.
- Posizioni e calibrazioni salvate in flash (NVS).

## Memoria del progetto
@docs/memoria/decisioni.md
@docs/memoria/punti-aperti.md

Quando si prende una nuova decisione o si chiude un punto aperto, aggiornare `docs/memoria/`
e la sezione corrispondente in `docs/tecnico/` o `docs/software/`.
