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
hardware/contenitore-rx/  scatole stampabili in 3D dell'antenna RX (motore ed elettronica; variabile e anello): CadQuery, STL e STEP
hardware/condensatore-tx/ disegno in sezione del condensatore a stantuffo TX con i collegamenti (matplotlib)
firmware/lib/Comune/   codice condiviso: protocollo ESP-NOW, bande, collegamento, chiavi
firmware/antenna/      firmware unità antenna (ESP32 DevKit 30 pin, WROOM-32), arduino-cli, unico per TX e RX
firmware/pannello/     firmware pannello in casa (Waveshare 7B), Arduino core 3.1.1 + LVGL 8.4
firmware/manopola/     manopola remota (Waveshare ESP32-S3-Knob-Touch-LCD-1.8): decoder dal microfono, ritocco fine, TUNE
firmware/lib/Decoder/  decoder CW/RTTY e decimatore (pannello e manopola)
firmware/lib/Grafica/  font con lettere accentate e cifre a 7 segmenti (pannello e manopola)
```
Il firmware del **pannello in casa** (Waveshare ESP32-S3-Touch-LCD-7B) ha tutte le schermate del mockup, non ancora
provate sulla scheda (stato in `firmware/pannello/README.md`). Requisiti e mockup approvati: `docs/software/03-unita-controllo.md`
(schermate, temi, screensaver), `08-decoder.md` (CW/RTTY) e `09-propagazione.md` (grey line, indici solari).
Font del pannello e della manopola: `firmware/lib/Grafica/src/font/` (Montserrat con Latin-1 e – — … − ↔ →). Nelle stringhe dell'interfaccia
usare solo quei caratteri; per aggiungerne altri modificare e rilanciare `firmware/pannello/strumenti/genera_font.sh`.

## Build
```bash
firmware/antenna/compila.sh            # antenna, firmware unico (tipo TX/RX dal pannello); "carica <porta>" carica
firmware/pannello/compila.sh           # pannello 7", con arduino-cli (core esp32 3.1.1, lvgl 8.4.0)
firmware/pannello/compila.sh 5b        # pannello 5" (ESP32-S3-Touch-LCD-5B: CH422G, porch diversi)
firmware/manopola/compila.sh           # manopola remota; "carica <porta>" carica (USB nativa)
firmware/pannello/test/esegui_test.sh  # test sul PC: sole, locator, indici, decoder CW/RTTY, QRZ, decimatore audio
firmware/pannello/simulatore/compila.sh <lvgl>  # interfaccia sul PC, schermate PNG (controllare dopo ogni modifica)
```
Le chiavi ESP-NOW reali vanno in `firmware/lib/Comune/src/segreti.h` (non versionato);
senza, si compila con le chiavi di esempio e un avviso.
Pannello e antenna usano il core Arduino ESP32 **3.1.1** con arduino-cli (gli esempi Waveshare usano `esp_lcd` e
`i2c_master` di IDF 5); `platformio.ini` dell'antenna non è più provato. Non usare `Wire` sul pannello: l'I2C è del
driver Waveshare. Sull'antenna `Wire` (SDA 21, SCL 22) è del display OLED, usato solo dal suo task.
Sul pannello LVGL alloca in **PSRAM** (`lv_conf.h`): non tornare a `malloc`, la RAM interna si esaurisce
(controllare con il comando `mem`). Nel task di LVGL (stack 6 KB) le strutture grandi vanno dichiarate `static`.
Schermo del pannello: clock dei pixel 21 MHz e bounce buffer da 15 righe; a 30 MHz l'immagine trema (PSRAM a 80 MHz).

## Regole che il firmware deve rispettare
- **Avvicinarsi alla posizione target sempre dallo stesso verso**: il gioco del riduttore
  (30 arc-min ≈ 2,7 pF) diventa un offset costante assorbito dalla tabella di calibrazione.
  Eccezione: il JOG libero della manopola (ritocco a orecchio) va diretto; il GOTO successivo riallinea.
- **Zero sui finecorsa a ogni cambio banda solo per il TX** (in ciclo aperto, senza encoder). L'RX (SERVO42D in
  ciclo chiuso) fa lo zero solo quando la posizione è sconosciuta: accensione, errore, stop durante lo zero, ZERO.
- **Mai impulsi sotto lo zero** (dove finisce l'azzeramento), tranne durante l'azzeramento stesso: controllo
  nell'ultima funzione di movimento di ciascun driver (`avvia` STEP/DIR, `muoviA` CAN).
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
