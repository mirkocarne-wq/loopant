# Decisioni di progetto

Registro delle scelte fatte e delle alternative scartate, con il motivo. Aggiornato al 20/11/2026.

## Prese
| Ambito | Decisione | Motivo |
|---|---|---|
| Bande | 40 m e 20 m | Obiettivo del progetto |
| Versioni | Due antenne: RX (1 m) e TX 100 W (1,5 m) | Il variabile a lamelle non regge il TX |
| Anello RX | Ø 1 m, rame ricotto 1/2" (o 5/8"), circa 3,3 m di tubo | In ricezione la perdita in più del tubo sottile è trascurabile |
| Anello TX | Ø 1,5 m, tubo 22 mm, rame | Corrente 9–13 A, serve bassa resistenza |
| Condensatore RX | Variabile ad aria 1000 pF "1000V CC" già in possesso | Disponibile; solo RX o prove a 1–2 W |
| Condensatore TX | Stantuffo tubo in tubo con manicotto PTFE | Nessun contatto strisciante; regge circa 4–5 kV di picco |
| Motore RX | NEMA17 + MKS SERVO42D CAN + riduttore planetario 20:1 | Closed loop, risoluzione circa 0,006 pF per conteggio |
| Motore TX | Attuatore lineare a guida, NEMA11, vite T6×1, corsa 150 mm | Già allineato, compatto |
| Driver TX | TMC2209 in stealthChop, STEP/DIR/EN + UART | StallGuard per rilevare blocchi, silenzioso |
| MCU antenna | ESP32 DevKit 38 pin, nella scatola all'antenna | Molti GPIO, ESP-NOW |
| Controllo casa | Waveshare ESP32-S3-Touch-LCD-7B (7", 1024×600) con LVGL | Schermo grande, touch capacitivo; sostituisce il 5" |
| Collegamento | ESP-NOW cifrato (PMK/LMK), MAC fissi in NVS, canale fisso (quello del router di casa se il pannello usa il WiFi) | Semplice; il WiFi serve solo per indici solari e ora |
| Alimentazione | 24 V 2,5 A in casa, convertitore 24→5 V all'antenna | Corrente sufficiente; alimentatore lontano dalla radio |
| Isolamento albero RX | Barra ABS 8 mm tra due giunti metallici | Albero e telaio del variabile portano RF |
| Schermate del pannello | Sintonia, Memorie, Propagazione, Decoder, Diagnostica, Impostazioni (+ Calibrazione da Impostazioni, screensaver) | Mockup approvato il 20/11/2026 |
| Temi del pannello | Notte, Giorno, Fosfori verdi, Fosfori ambra; STOP rosso in tutti | Leggibilità e gusto; lo STOP deve riconoscersi sempre |
| Numeri sul pannello | Display a 7 segmenti con segmenti spenti visibili | Aspetto da strumento, ottima leggibilità |
| Screensaver | Selezionabile: Orologio, Frequenza, Grey line, Spento | Richiesta dell'utente |
| Decoder sul pannello | CW e RTTY (RTTY con baud, shift, mark, stop, Reverse, UOS, AFC impostabili) | Richiesta dell'utente; fattibile sull'ESP32-S3 |
| Propagazione | Grey line e alba/tramonto calcolati in locale; indici solari via WiFi | La grey line non richiede Internet |
| Toolchain del pannello | Arduino, core ESP32 3.1.1 + LVGL 8.4 (arduino-cli o Arduino IDE 2) | Scelta dell'utente; è quella degli esempi Waveshare |
| Driver del pannello | Driver degli esempi Waveshare (15_LVGL_SLIDER) copiati in `src/waveshare/` | Funzionano sulla scheda; licenza Apache 2.0 |
| Cifre a 7 segmenti | Widget LVGL che disegna i segmenti con poligoni | Nessun font da convertire; stessa geometria del mockup |
| Font del pannello | Montserrat Medium 12-34 con Latin-1, generati con lv_font_conv; font integrati di LVGL disattivati | Testi in italiano corretto; circa 150 KB di flash in più |
| Ingresso audio del decoder | ADC1 su GPIO6 a 8 kHz (da verificare sullo schema) | Unico ADC1 apparentemente libero; nessun modulo in più |
| Algoritmi del decoder | C++ puro con test sul PC (segnali sintetici e rumore) | Si verificano senza hardware |
| Mappa della grey line | Disegnata in PSRAM con continenti semplificati; Natural Earth in futuro | LVGL 8 non riempie poligoni concavi |
| Simulatore sul PC | Interfaccia vera compilata con LVGL su Linux, schermate PNG | Controllo dell'impaginazione senza scheda |
| WiFi del pannello | Facoltativo: NTP e indici solari (hamqsl.com); il peer ESP-NOW segue il canale corrente | L'antenna deve stare sul canale del router |

## Scartate
| Alternativa | Motivo |
|---|---|
| SERVO42D per il TX | Sostituito dall'attuatore lineare NEMA11; resta sulla versione RX |
| NEMA17 + TR8×2 + guida MGN9H 300 mm per il TX | Valutata, non scelta (si preferisce il modulo lineare già pronto) |
| Variabile a lamelle 1000 pF in TX | Spaziatura circa 1 mm: inadatto a circa 3,3–4,7 kV di picco |
| Tubo prolunga doccia Leroy Merlin (ottone, circa 24 mm) | Scartato per il condensatore TX |
| diymore ESP32-S3 3,5" come unità antenna | Pochi GPIO, schermo inutile all'antenna |
| Calibrazione come scheda del pannello | Si usa di rado: spostata dentro Impostazioni |
