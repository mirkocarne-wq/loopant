# Decisioni di progetto

Registro delle scelte fatte e delle alternative scartate, con il motivo. Aggiornato al 26/11/2026.

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
| MCU antenna | ESP32 DevKit 30 pin (WROOM-32), già in possesso, nella scatola all'antenna | GPIO sufficienti, CAN, ADC1, modulo schermato contro l'RF del loop |
| Pannello da 5" | Waveshare ESP32-S3-Touch-LCD-5B, stesso firmware con `compila.sh 5b` (CH422G, porch del 5B, luminosità non regolabile) | Richiesta dell'utente; stessa risoluzione, nessuna schermata da rifare |
| Controllo casa | Waveshare ESP32-S3-Touch-LCD-7B (7", 1024×600) con LVGL | Schermo grande, touch capacitivo; sostituisce il 5" |
| Collegamento | ESP-NOW cifrato (PMK/LMK), MAC fissi in NVS, canale fisso (quello del router di casa se il pannello usa il WiFi) | Semplice; il WiFi serve solo per indici solari e ora |
| Alimentazione | 24 V 2,5 A in casa, convertitore 24→5 V all'antenna | Corrente sufficiente; alimentatore lontano dalla radio |
| Firmware dell'antenna | Unico per TX e RX: tipo scelto dal pannello (o `tipo tx\|rx`), salvato in NVS, riavvio al cambio; calibrazione separata per tipo; al primo avvio RX | Richiesta dell'utente; niente versioni da scegliere quando si carica |
| Finecorsa | Antenna RX: sugli ingressi del SERVO42D, che fa lo zero; antenna TX: sull'ESP32 (GPIO 27/14), zero fatto dal firmware | Decisione dell'utente; il TMC2209 non ha ingressi per i finecorsa |
| Finecorsa RX: zero e limite CW | Rimappatura 9E (zero su En, limite CW su Dir, COM a EVCC), EndLimit attivo, HmTrig High; il firmware li controlla all'avvio e scrive solo se serve | Il 42D ha un solo ingresso (IN_1): senza rimappatura il limite CW non veniva letto |
| Velocità RX | Movimenti a 60 giri/min del motore (prima 120), zero a 30 (prima 60), regolabili dalla console; rampa acc = 2 | Richiesta dell'utente: troppo veloce; lo zero del SERVO42D non ha rampa |
| Zero al cambio banda | Solo TX; l'RX fa lo zero solo con la posizione sconosciuta (accensione, errore, stop durante lo zero) o con ZERO | Decisione dell'utente (26/11/2026): il SERVO42D è in ciclo chiuso apposta; lo zero costava fino a circa 20 s a ogni cambio |
| Posizione RX dopo uno stop | Impulsi dallo zero (0x33), non l'encoder 0x31 | 0x31 conta dall'accensione del driver: posizione sbagliata dopo uno stop |
| Driver del motore | Segue il tipo di antenna: TX in STEP/DIR (TMC2209), RX via CAN (SERVO42D) | Precisazione dell'utente: sono due modelli di antenna; nessuna scelta separata |
| Transceiver CAN | Waveshare SN65HVD230 (3,3 V) su GPIO 17 (TX2) / 16 (RX2), già in possesso | Stessa tensione dell'ESP32, nessun convertitore di livello; 22/23 lasciati al display |
| Display all'antenna | OLED 0,96" SSD1306 (ELEGOO) su I2C SDA 21 / SCL 22, facoltativo, spento dopo 2 minuti a riposo | Stato e MAC visibili sul banco e all'installazione; già cablato sui pin I2C standard |
| EN del motore | GPIO 23 (prima 21) | Il 21 va al display; il 23 è libero all'accensione, il 4 ha una pull-down |
| Toolchain dell'antenna | arduino-cli con il core esp32 3.1.1, come il pannello (`compila.sh`) | Un solo core sul Mac; evita PlatformIO dopo l'avviso malware; compila senza modifiche |
| Pulsante ZERO sul pannello | In Sintonia accanto a TUNE e in Calibrazione nel riquadro della posizione | Mancava: c'era solo nella pagina web e in console |
| Associazione | Pulsante BOOT (GPIO 0) dell'antenna premuto 3 s, ricerca su tutti i canali, conferma con la finestra «Antenna trovata» sul pannello; l'antenna già associata si conferma da sola; protocollo v4 | Richiesta dell'utente; niente MAC da copiare e il canale del router si trova da solo |
| Canale ESP-NOW dell'antenna | Ricerca automatica quando il pannello tace (messaggio di stato su ogni canale, vince quello con l'ACK del pannello), salvato in NVS; anche `canale` da console | Il router cambia spesso canale; meno di un secondo, senza password del WiFi |
| Fissaggio del gruppo motore RX | Squadra a 90° tra riduttore (davanti) e SERVO42D (dietro), base della squadra verso il riduttore, motore su un appoggio con fascetta, avvitata alla base, sull'asse centrale della scatola; gruppo con il giunto lungo circa 150 mm; asole della squadra a 30 mm di interasse, viti dal basso con testa e rondella in un'asola larga sotto la piastra | Indicazione dell'utente |
| Concentricità motore–variabile RX | Asse comune a 60 mm dal fondo interno delle due scatole (`ASSE_Z`), come nella scatola e nella piastra del motore già stampate (rialzo 20 mm): 2 mm di spessore sotto la squadra; colonnine da 11 mm sotto la piastra del variabile | Misure dell'utente: perno del riduttore 52 mm sopra la faccia di sotto della piastra, variabile 45 mm sopra la sua base; nessuna ristampa |
| Finecorsa RX | Due sensori ottici a forcella dentro la scatola del motore, sotto l'albero, su un supporto stampato; ruota con due bandierine sull'albero d'uscita del riduttore (scatti 5° prima dei fermi: B 180° dopo A) | Escursione del variabile 190° (indicazione dell'utente); sensori già in possesso dell'utente |
| Scatola del variabile RX | Separata, sopra la scatola del motore sullo stesso palo, stessa costruzione (fondo, staffa, coperchio, borchia); interno largo 105 mm; regge l'anello: capi del tubo in un blocco pieno in alto, una vite M4 di traverso per capo, staffa allungata sotto il blocco | Richiesta dell'utente; alberi concentrici per costruzione; peso dell'anello portato al palo |
| Fissaggio del variabile RX | 2 viti M5 dal basso, attraverso asole della piastra, nei fori filettati delle linguette di sotto (114 mm) | Fori M5 già presenti sul variabile (misure dell'utente) |
| Uscita dell'albero RX | Foro da 8 mm al centro della parete frontale per il tubo isolante | Indicazione dell'utente |
| Contenitore RX sul palo | Staffa a V stampata con fascette stringitubo, fondo contro il palo, albero in alto | Richiesta dell'utente; la V va bene per pali di diametri diversi |
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
| Manopola remota | Waveshare ESP32-S3-Knob-Touch-LCD-1.8: decoder CW/RTTY dal microfono PDM, ritocco fine (JOG), frequenza e TUNE, display ruotato di 180°; passa sempre dal pannello; associazione automatica senza pulsante | Scatoletta vicino alla radio; un solo «capo» per l'antenna |
| Riferimento del ritocco fine | Relativo: riparte da 0 a ogni GOTO o ZERO; lo tiene il pannello e lo manda alla manopola | Richiesta dell'utente: la posizione assoluta non serve per ritoccare |
| Fine dell'azzeramento | Nessun movimento dopo lo zero: l'antenna resta lì fino al primo TUNE e quella posizione vale 0 (origine delle posizioni pubbliche e della calibrazione); il jog libero scende fino a 0, GOTO e jog normale restano sopra `posMin` | Richiesta dell'utente; prima il primo jog dopo lo zero saltava a `posMin` (532 passi) |
| Menu della manopola | Tocco lungo nel ritocco fine: Decoder, Luminosità, Rotazione, Associa a un pannello, Informazioni (Dimentica il pannello), Esci; luminosità e rotazione in NVS | Richiesta dell'utente: cambiare pannello (7″/5″) senza console |
| Decoder acceso/spento | Stato unico tenuto dal pannello (NVS), comandabile da pannello e manopola; da spento nessuna elaborazione | Richiesta dell'utente |
| Ritocco fine della manopola | JOG libero (senza recupero del gioco), 1-500 passi per scatto, predefinito 5; calibrazione e GOTO restano con il recupero | Con il recupero ogni scatto all'indietro faceva andata e ritorno |
| Librerie comuni | `lib/Decoder` (decoder e decimatore) e `lib/Grafica` (font e 7 segmenti) per pannello e manopola; collegamento ESP-NOW con un'istanza per peer | Stesso codice sulle due schede |
| Ingresso audio del decoder | ADC1 su GPIO6 (connettore J8) con circuito d'ingresso: trasformatore 1:1, MCP6002, passa-basso a 3,4 kHz; ADC a 32 kHz con FIR e riduzione a 8 kHz | Unico ADC1 libero (J8); qualità sufficiente per decodificare, pochi euro, nessuna scheda in più |
| Algoritmi del decoder | C++ puro con test sul PC (segnali sintetici e rumore) | Si verificano senza hardware |
| Mappa della grey line | Disegnata in PSRAM con continenti semplificati; Natural Earth in futuro | LVGL 8 non riempie poligoni concavi |
| Simulatore sul PC | Interfaccia vera compilata con LVGL su Linux o macOS, schermate PNG | Controllo dell'impaginazione senza scheda |
| Rete WiFi del pannello | Scelta con «Cerca reti» (elenco, tocco, password); niente ricerca con l'antenna in movimento | Più comodo che digitare l'SSID; la ricerca cambia canale e interrompe ESP-NOW |
| QRZ.com | Ricerca nominativi (XML Data) con o senza abbonamento; logbook (Logbook API) con abbonamento; HTTPS con certificati verificati | Richiesta dell'utente |
| Schede del pannello | Sette: aggiunta QRZ tra Decoder e Diagnostica | Spazio sufficiente con icona 24 px |
| Memoria di LVGL | In PSRAM con heap_caps_malloc | Con malloc la RAM interna si esauriva (820 byte liberi) |
| Pagina web | Server sulla porta 80 + mDNS loopant.local; stato, comandi, schermo in BMP, tocco remoto; senza password | Richiesta dell'utente; rete di casa fidata |
| Console | Su entrambe le USB-C (USB CDC On Boot + Serial0) | L'utente ha due porte; la USB nativa non riavvia la scheda |
| Schermo del pannello | Clock dei pixel 21 MHz, bounce buffer 15 righe, riallineamento del DMA a ogni quadro | A 30 MHz l'immagine tremava (PSRAM a 80 MHz nel core standard); provato sulla scheda il 21/11/2026 |
| WiFi del pannello | Facoltativo: NTP e indici solari (hamqsl.com); il peer ESP-NOW segue il canale corrente | L'antenna deve stare sul canale del router |

## Scartate
| Alternativa | Motivo |
|---|---|
| SERVO42D per il TX | Sostituito dall'attuatore lineare NEMA11; resta sulla versione RX |
| NEMA17 + TR8×2 + guida MGN9H 300 mm per il TX | Valutata, non scelta (si preferisce il modulo lineare già pronto) |
| Variabile a lamelle 1000 pF in TX | Spaziatura circa 1 mm: inadatto a circa 3,3–4,7 kV di picco |
| Tubo prolunga doccia Leroy Merlin (ottone, circa 24 mm) | Scartato per il condensatore TX |
| diymore ESP32-S3 3,5" come unità antenna | Pochi GPIO, schermo inutile all'antenna |
| Core Arduino "esp32-XIP-3.1.1" di Waveshare (PSRAM a 120 MHz) | Pacchetto non ufficiale da Google Drive; a 21 MHz lo schermo è già fermo |
| Bounce buffer da 20 righe | Schermo fermo, ma 20 KB di RAM interna in più rispetto a 15 righe, che bastano |
| PlatformIO per l'antenna (core 2.0.17) | Avviso malware di macOS al primo download, causa non trovata; sostituito da arduino-cli |
| Display sui pin 16/17 con EN e CAN fermi | Scelta dell'utente: il display resta sui pin I2C standard |
| Password del WiFi all'antenna con l'associazione | Non serve: la ricerca con ACK trova il canale; l'antenna non entra nella rete e la password non resta nella scatola sul balcone |
| Doppio driver per l'RX (STEP/DIR o CAN scelto dal pannello) | L'RX accorda solo via CAN; la scelta separata creava confusione con i finecorsa |
| Nodo audio con ESP32 classico, INMP441 e PCM1808 | Sostituito dalla manopola Waveshare, che ha già microfono, display e manopola |
| ESP32 Audio Kit (ES8388) | Non disponibile |
| Pannello CrowPanel ESP32-P4 7" | ESP-NOW non supportato dal C6 con il firmware di fabbrica; modulo C6 non riprogrammabile dal P4 |
| Telecomando diretto all'antenna (manopola senza pannello) | Due «capi» per l'antenna; scelto il passaggio dal pannello |
| ADC su I2C per l'audio | Troppo lento (ADS1115 860 campioni/s) o a 12 bit come l'ADC interno; bus condiviso con il touch |
| Calibrazione come scheda del pannello | Si usa di rado: spostata dentro Impostazioni |
| D1 mini ESP8266 all'antenna | Non è un ESP32: un solo ADC, pochi GPIO, niente CAN; FastAccelStepper non lo supporta |
| ESP32-S3-Zero all'antenna | Chip non schermato e antenna ceramica: troppo esposto all'RF del TX |
| ESP32-S3 N16R8 all'antenna | Memoria inutile qui; la PSRAM occupa GPIO 35-37 |
