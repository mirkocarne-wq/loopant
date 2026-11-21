# Pannello in casa — Waveshare ESP32-S3-Touch-LCD-7B

Firmware Arduino (core ESP32 **3.x**) + LVGL 8.4. Requisiti: [docs/software/03-unita-controllo.md](../../docs/software/03-unita-controllo.md).

## Stato
| Parte | Stato |
|---|---|
| Schermo RGB, touch GT911, LVGL (driver Waveshare) | scritto, compila |
| Collegamento ESP-NOW con l'antenna: heartbeat, ripetizioni, stato | scritto, compila |
| Barra in alto (nominativo, orologio, antenna, stato, STOP) e schede | scritto, compila |
| Sintonia | scritta, compila |
| Impostazioni: nominativo, locator, tema, MAC | prima versione |
| Memorie, Propagazione, Decoder, Diagnostica, Calibrazione, screensaver | segnaposto |
| Console seriale | scritta |

**Non ancora provato sulla scheda.**

## Struttura
```
pannello.ino            setup/loop
lv_conf.h               configurazione LVGL (da quella Waveshare)
compila.sh              compilazione e caricamento con arduino-cli
src/config.h            canale, tempi del collegamento, frequenze proposte
src/impostazioni.*      impostazioni in NVS (nominativo, tema, MAC antenna, ultime frequenze...)
src/tema.*              i quattro temi del mockup
src/display7.*          widget display a 7 segmenti (poligoni LVGL, nessun font esterno)
src/link_antenna.*      ESP-NOW verso l'antenna (usa ../lib/Comune)
src/console.*           comandi da seriale
src/ui/                 barra, schede e schermate
src/waveshare/          driver degli esempi Waveshare (Apache 2.0, vedi NOTICE.md)
```

## Compilare con arduino-cli
```bash
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
```bash
arduino-cli core update-index && arduino-cli core install esp32:esp32@3.1.1
```
```bash
arduino-cli lib install lvgl@8.4.0
```
```bash
./compila.sh
```
```bash
./compila.sh carica /dev/ttyACM0
```
La porta è quella del connettore USB-C "USB TO UART" (CH343); il nome cambia da sistema a sistema.

## Compilare con Arduino IDE 2
1. Gestore schede: **esp32 di Espressif Systems, versione 3.1.1**.
2. Gestore librerie: **lvgl 8.4.0**.
3. Copiare `lv_conf.h` nella cartella `libraries` dello sketchbook, accanto alla cartella `lvgl`.
4. Rendere visibile la libreria comune: copiare (o collegare) `firmware/lib/Comune` in `libraries/Comune`.
5. Scheda **ESP32S3 Dev Module** con: PSRAM **OPI PSRAM**, Flash Size **16MB**, Flash Mode **QIO 80MHz**,
   Partition Scheme **16M Flash (3MB APP/9.9MB FATFS)**, USB CDC On Boot **Disabled**.
6. Aprire `pannello.ino` e caricare.

## Primo avvio
Dalla console seriale (115200 baud):
```
mac AA:BB:CC:DD:EE:FF        MAC dell'unità antenna (lo stampa l'antenna all'accensione)
call <nominativo>            nominativo (o dalla schermata Impostazioni)
ora 2026-11-21 14:30         ora UTC, finché non c'è l'NTP
```
Sull'antenna va impostato il MAC del pannello (stampato all'avvio e mostrato in Impostazioni).

## Limiti noti di questa versione
- I font Montserrat inclusi in LVGL hanno solo i caratteri ASCII: niente lettere accentate, "·" e "—".
  Vanno generati con `lv_font_conv` includendo il set Latin-1.
- Icone delle schede: per ora i simboli integrati in LVGL; quelle del mockup vanno convertite in immagini.
- Temi a fosfori: niente bagliore e righe di scansione (costosi in LVGL), niente font monospazio.
- La luminosità (PWM dell'IO expander) non è ancora usata: il verso del valore è da verificare sulla scheda.
