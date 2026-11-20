# 03 — Pannello in casa (unità di controllo): requisiti

Scheda: **Waveshare ESP32-S3-Touch-LCD-7B** (7", 1024×600, touch capacitivo).
Fonti: [documentazione Waveshare](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B), wiki Waveshare
ed esempi ufficiali (repository `waveshareteam/ESP32-S3-Touch-LCD-7B`, licenza Apache 2.0).

Legenda: **[P]** = già deciso nel progetto; **[N]** = proposta nuova, da confermare.

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
| Altre interfacce | CAN (TJA1051, GPIO 19/20), RS485 (TX 16 / RX 15), SD (MOSI 11, SCK 12, MISO 13), connettori I2C, UART, sensore |
| Batteria | Li-ion 3,7 V opzionale, connettore PH2.0, caricatore integrato |
| Alimentazione | 5 V da USB-C, circa 350 mA |
| Software Waveshare | Arduino core ESP32 **3.x** (3.0.7 / 3.1.1), LVGL 8.4 (anche 9.x), driver `esp_lcd` RGB con doppio framebuffer in PSRAM |

## B. Requisiti funzionali
| ID | Requisito | |
|---|---|---|
| F1 | Inserire la frequenza da tastierino numerico, **in kHz** con un decimale | [P] |
| F2 | Pulsanti **40 m** e **20 m** che propongono una frequenza della banda | [P] |
| F3 | **TUNE**: invia la frequenza all'antenna, che va in risonanza | [P] |
| F4 | **STOP**: ferma subito il motore | [P] |
| F5 | **ZERO**: azzeramento sui finecorsa | [P] |
| F6 | Mostrare lo **stato**: fermo / in movimento / azzeramento / errore (con il motivo) | [P] |
| F7 | Mostrare la **posizione** del motore | [P] |
| F8 | Mostrare la frequenza su cui è accordata l'antenna, distinta da quella che si sta digitando | [N] |
| F9 | Rifiutare le frequenze fuori banda prima dell'invio (7,000–7,200 e 14,000–14,350 MHz) | [N] |
| F10 | Ricordare l'ultima frequenza usata per ogni banda e riproporla con il pulsante della banda | [N] |
| F11 | Passi ±: pulsanti di regolazione fine (es. ±1 kHz, ±5 kHz) che riaccordano subito | [N] |
| F12 | **Pagina calibrazione**: jog del motore (±10 / ±100 / ±1000 passi), "salva punto" alla frequenza indicata, elenco e cancellazione dei punti per banda | [N] |
| F13 | Pagina **impostazioni**: MAC dell'antenna, luminosità, spegnimento dello schermo dopo N minuti | [N] |

## C. Collegamento con l'antenna
| ID | Requisito | |
|---|---|---|
| C1 | ESP-NOW, MAC fissi salvati in NVS, stesso canale WiFi, nessun router | [P] |
| C2 | Cifratura con chiavi PMK/LMK | [P] |
| C3 | Heartbeat ogni secondo | [P] |
| C4 | Indicatore di collegamento (OK / perso dopo 3 s senza risposte) | [N] |
| C5 | Comandi `GOTO`, `STOP`, `ZERO`, `STATO`; risposta con posizione, stato e numero di sequenza | [P] |
| C6 | Ripetere il comando se non arriva la risposta (max 3 tentativi) | [N] |
| C7 | Pairing con pulsante (scambio dei MAC in broadcast) | [P] opzionale |

## D. Sicurezza in TX
| ID | Requisito | |
|---|---|---|
| S1 | Lo STOP è sempre visibile e raggiungibile da ogni pagina | [N] |
| S2 | Promemoria a schermo: non accordare con RF ad alta potenza (circa 5 W per la ricerca del ROS) | [P] |
| S3 | Con il collegamento perso, TUNE disabilitato e avviso evidente | [N] |

## E. Requisiti non funzionali
| ID | Requisito | |
|---|---|---|
| N1 | Interfaccia in italiano, caratteri grandi e leggibili a distanza (frequenza ≥ 48 px) | [N] |
| N2 | Tasti grandi, usabili con il dito (≥ 80 px) | [N] |
| N3 | Nessun disturbo HF dal pannello vicino alla radio: da verificare in ascolto; eventuale ferrite sul cavo USB | [N] |
| N4 | Si alimenta dalla USB-C a 5 V; batteria non necessaria | [N] |
| N5 | Console seriale con gli stessi comandi, per prove senza touch | [N] |
| N6 | Base software: Arduino core ESP32 3.x + LVGL 8.4, partendo dai driver degli esempi Waveshare | [N] |

## F. Sviluppi futuri
| ID | Requisito | |
|---|---|---|
| X1 | CI-V (Icom IC-7300 MK2) / CAT (Yaesu): "segui la radio", con riaccordo automatico al cambio di frequenza | [P] |
| X2 | Visualizzazione di potenza e ROS dal wattmetro (AD8307 + ADS1115) con allarmi | [P] |
| X3 | Grafico del ROS durante la ricerca del picco | [N] |

## G. Domande aperte
1. **UART per il CI-V**: il connettore UART condivide la seriale con il CH343 (selettore sulla scheda) e
   l'RS485 usa GPIO 15/16. Quale usare per il CI-V? Da verificare sullo schema elettrico.
2. **Toolchain**: gli esempi Waveshare richiedono il core Arduino 3.x. La piattaforma PlatformIO ufficiale
   provata qui (espressif32 7.1.3) ha ancora il core 2.0.17. Scelte possibili: Arduino IDE 2,
   PlatformIO con la piattaforma pioarduino, oppure ESP-IDF.
3. Schermo sempre acceso o spegnimento automatico?
4. Pannello fisso alla parete/scrivania o anche a batteria?
