# Pannello in casa — Waveshare ESP32-S3-Touch-LCD-7B

Firmware Arduino (core ESP32 **3.x**) + LVGL 8.4. Requisiti: [docs/software/03-unita-controllo.md](../../docs/software/03-unita-controllo.md).

## Stato
Tutte le schermate del mockup sono scritte. **Provato sulla scheda il 21/11/2026**: schermo, touch, colori,
luminosità e ricerca delle reti WiFi funzionano. Da provare: collegamento con l'antenna, decoder con segnali veri.

| Parte | Stato |
|---|---|
| Schermo RGB, touch GT911, LVGL (driver Waveshare) | scritto |
| Collegamento ESP-NOW con l'antenna (protocollo v2): heartbeat, ripetizioni, stato | scritto |
| Barra in alto (nominativo, orologio UTC/locale, antenna, stato, STOP) e schede | scritto |
| Sintonia | scritta |
| Memorie: predefinite e personali (NVS), filtri, nuova, elimina | scritta |
| Propagazione: mappa con grey line, alba e tramonto, indici solari via WiFi | scritta (mappa semplificata) |
| Decoder CW e RTTY con ingresso audio dall'ADC | scritto; algoritmi provati sul PC |
| Diagnostica: collegamento, motore, finecorsa, contatori, eventi | scritta |
| Impostazioni: stazione, tema, antenna, screensaver, luminosità, ora, WiFi | scritte |
| Calibrazione (da Impostazioni) | scritta |
| Screensaver Orologio, Frequenza, Grey line, Spento | scritto |
| WiFi facoltativo: NTP e indici solari (hamqsl.com) | scritto |
| QRZ.com: ricerca nominativi (con o senza abbonamento), i miei QSO | scritta; HTTPS provato, da provare con un account |
| Pagina web (stato, comandi, vista e tocco dello schermo) | scritta e provata dal Mac |
| Console seriale, su entrambe le USB-C | scritta |

## Struttura
```
pannello.ino            setup/loop
lv_conf.h               configurazione LVGL (da quella Waveshare)
compila.sh              compilazione e caricamento con arduino-cli
src/config.h            canale, tempi del collegamento, frequenze proposte
src/impostazioni.*      impostazioni in NVS (nominativo, tema, MAC antenna, ultime frequenze...)
src/tema.*              i quattro temi del mockup
src/display7.*          widget display a 7 segmenti (poligoni LVGL, nessun font esterno)
src/icona.*             icone delle schede disegnate con linee e archi (come nel mockup)
src/link_antenna.*      ESP-NOW verso l'antenna (usa ../lib/Comune)
src/console.*           comandi da seriale
src/eventi.*            registro eventi e contatori (Diagnostica)
src/memorie.*           memorie predefinite e personali
src/sole.* locator.*    posizione del sole, alba/tramonto, locator Maidenhead
src/mappa.*             mappa con grey line disegnata in PSRAM (mappa_dati.h: continenti semplificati)
src/indici.* rete.*     indici solari, WiFi, NTP
src/ora.* schermo.*     ora UTC/locale, retroilluminazione
src/audio.*             ingresso audio (ADC continuo a 8 kHz)
src/qrz.* geo.*         risposte di QRZ (C++ puro), distanza e direzione
src/servizio_qrz.*      QRZ.com via HTTPS (task sul core 0)
src/server_web.* pagina_web.h tocco_remoto.*  pagina web e tocco da remoto
src/decoder/            algoritmi CW e RTTY (C++ puro, provati sul PC)
src/decodifica.*        decoder sul pannello: impostazioni, testo ricevuto
test/                   test sul PC: ./test/esegui_test.sh
simulatore/             interfaccia sul PC con LVGL, schermate in PNG
src/font/               font Montserrat 12-34 con lettere accentate (generati, non modificare a mano)
strumenti/genera_font.sh  rigenera i font con lv_font_conv 1.5.3
src/ui/                 barra, schede e schermate
src/waveshare/          driver degli esempi Waveshare (Apache 2.0, vedi NOTICE.md)
```

## Compilare con arduino-cli
Funziona su Linux e macOS (provato con arduino-cli 1.6.0 da Homebrew).
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
Si può usare una qualsiasi delle due USB-C: "USB TO UART" (CH343) o la USB nativa. Il nome della porta
cambia da sistema a sistema (su macOS `/dev/cu.usbmodem…`).

## Compilare con Arduino IDE 2
1. Gestore schede: **esp32 di Espressif Systems, versione 3.1.1**.
2. Gestore librerie: **lvgl 8.4.0**.
3. Copiare `lv_conf.h` nella cartella `libraries` dello sketchbook, accanto alla cartella `lvgl`.
4. Rendere visibile la libreria comune: copiare (o collegare) `firmware/lib/Comune` in `libraries/Comune`.
5. Scheda **ESP32S3 Dev Module** con: PSRAM **OPI PSRAM**, Flash Size **16MB**, Flash Mode **QIO 80MHz**,
   Partition Scheme **16M Flash (3MB APP/9.9MB FATFS)**, USB CDC On Boot **Enabled**.
6. Aprire `pannello.ino` e caricare.

## Test sul PC
```bash
test/esegui_test.sh
```
Prova i calcoli del sole e del locator, l'analisi del feed degli indici e i decoder CW e RTTY
con segnali sintetici e rumore (velocità automatica, filtri, Reverse, 50 baud/850 Hz, AFC con 25 Hz di errore).

## Simulatore e schermate
Il simulatore ([simulatore/README.md](simulatore/README.md)) disegna le schermate vere sul PC.
Le ultime sono in [docs/software/schermate/](../../docs/software/schermate/).

## Ingresso audio
Il decoder legge l'audio della radio da `cfg::PIN_AUDIO` (predefinito **GPIO6**, l'unico ADC1 che sembra libero
sul 7B: **da verificare sullo schema**). Collegamento: [docs/tecnico/06-elettronica.md](../../docs/tecnico/06-elettronica.md).
Con `PIN_AUDIO = -1` il decoder è disattivato.

## WiFi (facoltativo)
Serve solo per l'ora esatta (NTP) e gli indici solari. In Impostazioni: **Cerca reti**, si tocca la rete e si inserisce
la password; oppure dalla console (`reti`, `wifi <ssid> <password>`). L'ESP32-S3 vede solo le reti a 2,4 GHz.
Durante la ricerca la radio cambia canale e il collegamento con l'antenna si interrompe per qualche secondo:
con l'antenna in movimento la ricerca non parte.
Con il WiFi collegato il canale ESP-NOW diventa quello del router: **sull'antenna `CANALE_WIFI` deve essere uguale**.

## Memoria
LVGL alloca i suoi oggetti in **PSRAM** (`lv_conf.h`, `heap_caps_malloc`). Con `malloc` normale le migliaia di
piccoli oggetti finivano in RAM interna (allocazioni sotto i 4 KB) e la esaurivano: WiFi, DNS e HTTPS fallivano.
Non tornare a `malloc`. Il comando `mem` mostra la RAM interna libera (circa 137 KB a riposo).

## Font
I font in `src/font/` sono Montserrat Medium (SIL Open Font License 1.1) con i simboli di LVGL
(Font Awesome 5 Free, SIL OFL 1.1), presi dalla libreria lvgl 8.4 (`scripts/built_in_font/`).
Contengono ASCII, Latin-1 (à è é ì ò ù · ° ×) e – — ’ … − → ↔. Per aggiungere caratteri:
```bash
npm install lv_font_conv@1.5.3
```
```bash
LV_FONT_CONV=node_modules/.bin/lv_font_conv strumenti/genera_font.sh <percorso>/lvgl/scripts/built_in_font
```

## Primo avvio
Dalla console seriale (115200 baud):
```
mac AA:BB:CC:DD:EE:FF        MAC dell'unità antenna (lo stampa l'antenna all'accensione)
call <nominativo>            nominativo (o dalla schermata Impostazioni)
ora 2026-11-21 14:30         ora UTC, se non c'è il WiFi
wifi <ssid> <password>       WiFi di casa (facoltativo)
```
Sull'antenna va impostato il MAC del pannello (stampato all'avvio e mostrato in Impostazioni).

## Limiti noti di questa versione
- Temi a fosfori: niente bagliore e righe di scansione (costosi in LVGL), niente font monospazio.
- Mappa: continenti molto semplificati, da sostituire con dati Natural Earth.
- Indici solari: formato del feed hamqsl.com e condizioni d'uso da verificare.
- Impostazioni salvate in formato v2: al primo avvio con questa versione si torna ai valori predefiniti.
