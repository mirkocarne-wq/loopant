# 03 — Pannello in casa (unità di controllo): requisiti

Scheda: **Waveshare ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo).
Fonti: [documentazione Waveshare](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B), wiki Waveshare
ed esempi ufficiali (repository `waveshareteam/ESP32-S3-Touch-LCD-7B`, licenza Apache 2.0).

**Mockup approvato** (20/11/2026): canvas "Pannello MagLoop 7B" su claude.ai
(<https://claude.ai/artifact/99tPobvTpdkJt4b83kAaNP>, privato: visibile solo al proprietario).

Legenda: **[D]** = deciso (anche tramite il mockup); **[N]** = proposta tecnica, da confermare.
I valori mostrati nel mockup (posizioni, eventi, indici solari) sono **di esempio**.

Sezioni collegate: [08 — Decoder CW/RTTY](08-decoder.md), [09 — Propagazione](09-propagazione.md).

**Schermate del firmware** (generate dal simulatore sul PC, dati di esempio): [schermate/](schermate/).

## A. Dati della scheda (da documentazione Waveshare)
| Voce | Valore |
|---|---|
| Modulo | ESP32-S3-WROOM-1-N16R8: 16 MB flash, 8 MB PSRAM ottale |
| Schermo | IPS 1024×600, RGB parallelo 16 bit (RGB565), PCLK 30 MHz negli esempi |
| Pin RGB | PCLK 7, HSYNC 46, VSYNC 3, DE 5; R3–R7 = 1, 2, 42, 41, 40; G2–G7 = 39, 0, 45, 48, 47, 21; B3–B7 = 14, 38, 18, 17, 10 |
| Temporizzazioni (esempi) | HSYNC 162 / HBP 152 / HFP 48; VSYNC 45 / VBP 13 / VFP 3; PCLK attivo basso |
| Touch | GT911, I2C su SDA 8 / SCL 9, IRQ GPIO 4, reset tramite IO expander |
| IO expander | Indirizzo I2C 0x24 (chip non indicato): EXIO1 reset touch, EXIO2 retroilluminazione, EXIO4 CS della SD, EXIO5 selezione USB/CAN, EXIO6 alimentazione LCD; ha anche un'uscita PWM e un ingresso ADC (tensione batteria) |
| Retroilluminazione | Accesa/spenta con EXIO2; luminosità tramite il PWM dell'IO expander (registro 0x05, 0–100) |
| USB | Due USB-C: USB nativa (GPIO 19/20, in comune con il CAN) e "USB TO UART" (CH343) |
| Altre interfacce | CAN (TJA1051, GPIO 19/20), RS485 (TX 16 / RX 15), SD (MOSI 11, SCK 12, MISO 13); connettori J8 «GPIO» (3V3, GND, GPIO6), H1 «I2C» (SDA 8, SCL 9), H3 «UART2» (GPIO 43/44, con SW1 su UART2 al posto del CH343) — dallo schema |
| Batteria | Li-ion 3,7 V opzionale, connettore PH2.0, caricatore integrato |
| Alimentazione | 5 V da USB-C, circa 350 mA |
| RTC / ingresso audio | **Non indicati** nella documentazione |
| Software Waveshare | Arduino core ESP32 **3.x** (3.0.7 / 3.1.1), LVGL 8.4 (anche 9.x), driver `esp_lcd` RGB con doppio framebuffer in PSRAM |

## B. Struttura delle schermate [D]
```
┌───────────────────────────────────────────────────────────────────────┐
│ NOMINATIVO  LOCATOR        14:32 UTC   ● Antenna   FERMO     [ STOP ] │  barra in alto (sempre)
├───────────────────────────────────────────────────────────────────────┤
│                         schermata attiva                              │
├───────────────────────────────────────────────────────────────────────┤
│ Sintonia │ Memorie │ Propag. │ Decoder │ QRZ │ Diagnostica │ Impostaz. │  schede (sempre)
└───────────────────────────────────────────────────────────────────────┘
```
- **Barra in alto** (64 px): nominativo e locator, orologio, stato del collegamento, stato dell'antenna, **STOP**.
- **Schede in basso** (72 px): sette schede con icona e nome (QRZ aggiunta il 21/11/2026).
- **Calibrazione** non ha una scheda: si apre da Impostazioni → "Calibrazione antenna".
- **Screensaver** dopo un tempo impostabile; un tocco riporta alla Sintonia.

## C. Requisiti per schermata

### C1. Sintonia
| ID | Requisito | |
|---|---|---|
| S1 | Frequenza da tastierino in **kHz** con un decimale, su **display a 7 segmenti** (5 cifre intere, posizioni vuote "spente") | [D] |
| S2 | Frequenza su cui è accordata l'antenna, distinta da quella che si sta digitando; posizione del motore | [D] |
| S3 | Segmento del band plan IARU Regione 1 per la frequenza digitata (CW, digitale, tutti i modi, radiofari; "FT8" vicino a 7 074 / 14 074) | [D] (band plan da verificare) |
| S4 | Pulsanti **40 m** / **20 m** (propongono l'ultima frequenza usata nella banda) | [D] |
| S5 | Tasti di passo **±1** e **±5 kHz** | [D] |
| S6 | **TUNE** invia la frequenza; disattivato se fuori banda (7 000–7 200, 14 000–14 350 kHz), con avviso | [D] |
| S7 | **ZERO** accanto a TUNE: azzeramento sul finecorsa a richiesta (il primo TUNE di ogni banda lo fa da solo); disattivato senza antenna | [D] |

### C2. Memorie
| ID | Requisito | |
|---|---|---|
| M1 | Memorie predefinite per 40 e 20 m: FT8, FT4, WSPR, QRP CW, QRP SSB (frequenze da verificare) | [D] |
| M2 | Memorie personali con un nome; "Nuova memoria" | [D] |
| M3 | Filtri Tutte / 40 m / 20 m / Mie; etichetta colorata per modo | [D] |
| M4 | Un tocco su una memoria accorda l'antenna | [D] |

### C3. Propagazione — vedi [09-propagazione.md](09-propagazione.md)
| ID | Requisito | |
|---|---|---|
| P1 | Mappa del mondo con notte, **grey line**, punto subsolare, stazione e alcune città, calcolata dall'ora UTC (senza Internet) | [D] |
| P2 | Alba e tramonto alla stazione (UTC), tempo al prossimo passaggio della grey line, fascia giorno/notte sulle 24 ore | [D] |
| P3 | Indici solari SFI, SSN, A, K e stima delle bande 40/20 m giorno/notte, **via WiFi** | [D] |

### C4. Decoder CW / RTTY — vedi [08-decoder.md](08-decoder.md)
| ID | Requisito | |
|---|---|---|
| D1 | Selettore di modo **CW / RTTY**; testo decodificato grande; Pulisci | [D] |
| D2 | CW: velocità WPM (automatica), tono, livello con soglia, ultimi elementi ricevuti, filtro 100 / 250 / 500 Hz | [D] |
| D3 | RTTY **impostabile**: 45,45 / 50 / 75 baud; shift 170 / 425 / 850 Hz; tono di mark regolabile (±1 / ±10 Hz), space calcolato; bit di stop 1 / 1,5 / 2; **Reverse**, **UOS**, **AFC**; indicatore di sintonia mark/space | [D] |

### C5. Diagnostica
| ID | Requisito | |
|---|---|---|
| G1 | Collegamento: stato, segnale (dBm), pacchetti persi, ultima risposta | [D] |
| G2 | Motore: posizione, driver, finecorsa MIN/MAX, ultimo azzeramento | [D] |
| G3 | Contatori: movimenti, azzeramenti, tempo di accensione, versione dell'antenna (TX/RX) | [D] |
| G4 | Ultimi eventi con ora e tipo (accordata, zero, errore) | [D] |
| G5 | Riquadro Potenza e ROS, attivo quando ci sarà il wattmetro | [D] |

### C6. Impostazioni
| ID | Requisito | |
|---|---|---|
| I1 | **Nominativo** e **locator** (mostrati nella barra in alto e nello screensaver; il locator dà la posizione per la propagazione) | [D] |
| I2 | **Tema**: scelta con miniature (sezione D) | [D] |
| I3 | Antenna: MAC, **tipo TX/RX** (firmware unico, segue quello dichiarato dall'antenna), per l'RX indirizzo e velocità del CAN (il driver segue il tipo: TX STEP/DIR, RX CAN), **Pairing**, accesso a **Calibrazione antenna** | [D] |
| I4 | **Screensaver**: stile Orologio / Frequenza / Grey line / Spento; parte dopo Mai / 5 / 15 / 30 min; anteprima | [D] |
| I5 | Luminosità; ora nella barra UTC / locale / entrambe | [D] |
| I6 | WiFi: pulsante **Cerca reti**, elenco delle reti trovate (segnale, protetta/aperta), tocco sulla rete, password con tastiera e collegamento; campo manuale per le reti nascoste | [D] |
| I7 | **Associazione**: quando un'antenna ha BOOT premuto compare su qualsiasi schermata la finestra «Antenna trovata» (TX/RX, MAC, segnale, antenna che sostituisce) con «Associa» e «Ignora»; l'antenna già associata si conferma da sola | [D] |
| I8 | **Manopola remota**: stato del collegamento, sorgente del decoder (Microfono / Segnale di prova), passi per scatto del ritocco fine ([12-manopola.md](12-manopola.md)) | [D] |

### C7. Calibrazione (da Impostazioni)
| ID | Requisito | |
|---|---|---|
| K1 | Scelta banda 40 / 20 m; posizione del motore su display a 7 segmenti, con pulsante **ZERO** (serve prima del jog) | [D] |
| K2 | Jog ±10 / ±100 / ±1000 passi; frequenza del dip letta sul NanoVNA; **Salva punto** | [D] |
| K3 | Grafico posizione ↔ frequenza e tabella dei punti con eliminazione | [D] |

### C7b. QRZ — vedi [10-qrz.md](10-qrz.md)
| ID | Requisito | |
|---|---|---|
| Q1-Q7 | Ricerca nominativi (con o senza abbonamento), distanza/direzione/percorso, i miei QSO con «Accorda», credenziali in Impostazioni | [D] |

### C7c. Pagina web — vedi [11-pagina-web.md](11-pagina-web.md)
| ID | Requisito | |
|---|---|---|
| W1 | Pagina web sulla rete di casa: stato, STOP, TUNE, ZERO, vista e tocco dello schermo | [D] |

### C8. Screensaver
| ID | Requisito | |
|---|---|---|
| V1 | **Orologio**: nominativo, orologio UTC grande, ora locale e data, frequenza accordata | [D] |
| V2 | **Frequenza**: frequenza accordata molto grande, banda e segmento, orologio piccolo | [D] |
| V3 | **Grey line**: mappa a tutto schermo con nominativo, alba/tramonto, frequenza e orologio | [D] |
| V4 | Un tocco riporta alla Sintonia | [D] |

## D. Temi [D]
| Tema | Aspetto |
|---|---|
| **Notte** (predefinito) | Fondo quasi nero, testo chiaro, **ambra** per numeri importanti e TUNE |
| **Giorno** | Fondo chiaro ad alto contrasto, ambra scura |
| **Fosfori verdi** | Monocromatico verde da monitor a fosfori, font a spaziatura fissa, bagliore e righe di scansione |
| **Fosfori ambra** | Come il precedente, in ambra |

- **Lo STOP è rosso in tutti i temi** (sicurezza).
- Numeri su **display a 7 segmenti** in ogni tema, con i segmenti spenti appena visibili.
- Contrasto del testo ≥ 4,5:1; i colori di stato si distinguono anche per luminosità.
- Font: Montserrat (integrato in LVGL) per Notte/Giorno; per i fosfori un font monospazio (nel mockup IBM Plex Mono)
  e per le cifre un font a 7 segmenti (es. DSEG7), entrambi da convertire con `lv_font_conv`. [N]

## E. Collegamento con l'antenna
| ID | Requisito | |
|---|---|---|
| E1 | ESP-NOW, MAC fissi salvati in NVS, stesso canale WiFi | [D] |
| E2 | Cifratura con chiavi PMK/LMK | [D] |
| E3 | Heartbeat ogni secondo; indicatore di collegamento (perso dopo 3 s senza risposte) | [D] |
| E4 | Comandi `GOTO`, `STOP`, `ZERO`, `STATO`, `JOG`, `CAL_SALVA`, `CAL_CANCELLA` (protocollo in [04](04-protocollo-espnow.md)) | [D] |
| E5 | Ripetere il comando se non arriva la risposta (max 3 tentativi) | [N] |
| E6 | Pairing con il pulsante BOOT dell'antenna (scambio dei MAC in broadcast, conferma sul pannello: I7); schermata `notte-1b-associazione` | [D] |
| E7 | Con il WiFi attivo (indici solari) il canale ESP-NOW è quello del router di casa, anche sull'antenna | [D] |

## F. Sicurezza
| ID | Requisito | |
|---|---|---|
| F1 | STOP sempre visibile e raggiungibile da ogni schermata | [D] |
| F2 | Con il collegamento perso, TUNE disattivato e avviso evidente | [D] |
| F3 | In TX non accordare con RF ad alta potenza (circa 5 W per la ricerca del ROS): promemoria a schermo | [D] |

## G. Requisiti non funzionali
| ID | Requisito | |
|---|---|---|
| N1 | Interfaccia in italiano; frequenza principale ≥ 84 px, testo ≥ 13 px | [D] |
| N2 | Tasti ≥ 44 px, quelli principali 54–80 px | [D] |
| N3 | Nessun disturbo HF dal pannello vicino alla radio: da verificare in ascolto; eventuale ferrite sul cavo USB | [N] |
| N4 | Alimentazione dalla USB-C a 5 V | [N] |
| N5 | Console seriale con gli stessi comandi, per prove senza touch | [N] |
| N6 | Base software: Arduino core ESP32 3.x + LVGL 8.4, partendo dai driver degli esempi Waveshare (Apache 2.0, citando la fonte) | [N] |
| N7 | Impostazioni (nominativo, tema, screensaver, memorie, decoder) salvate in NVS | [N] |

## H. Sviluppi futuri
| ID | Requisito | |
|---|---|---|
| X1 | CI-V (Icom IC-7300 MK2) / CAT (Yaesu): "segui la radio" | [D] |
| X2 | Potenza e ROS dal wattmetro (AD8307 + ADS1115) con allarmi | [D] |
| X3 | Grafico del ROS durante la ricerca del picco | [N] |

## I. Domande aperte
1. **UART per il CI-V**: il connettore UART condivide la seriale con il CH343 (selettore sulla scheda) e
   l'RS485 usa GPIO 15/16. Quale usare? Da verificare sullo schema elettrico.
2. **Ingresso audio per il decoder**: la scheda non ne documenta uno (vedi [08](08-decoder.md)).
3. **Ora esatta**: nessun RTC documentato. Fonti possibili: NTP via WiFi (già necessario per gli indici solari),
   impostazione manuale, in futuro CI-V dalla radio.
4. **Toolchain**: serve il core Arduino 3.x; la piattaforma PlatformIO ufficiale `espressif32` provata qui
   (7.1.3) ha ancora il core 2.0.17. Scelte: Arduino IDE 2, PlatformIO con pioarduino, oppure ESP-IDF.
5. Alimentazione: solo USB o anche batteria?
