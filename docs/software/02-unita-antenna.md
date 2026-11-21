# 02 — Unità antenna

Hardware: vedi [tecnico/06-elettronica.md](../tecnico/06-elettronica.md). Codice: `firmware/antenna/`.

## Versioni
**Firmware unico** per le due antenne. Si compila con arduino-cli e il core esp32 **3.1.1**, lo stesso del pannello:
`firmware/antenna/compila.sh` (con `carica <porta>` carica anche). Librerie: FastAccelStepper 2.0.1, TMCStepper 0.7.3,
U8g2. `platformio.ini` resta per chi usa PlatformIO (core 2.0.17), ma non è più provato.

| Tipo | Antenna | Motore | Driver |
|---|---|---|---|
| TX | TX 100 W | NEMA11, attuatore lineare T6×1, corsa 150 mm | TMC2209 (STEP/DIR/EN + UART) |
| RX | RX | NEMA17 + riduttore 20:1 sul variabile | SERVO42D in STEP/DIR/EN **oppure** via CAN, a scelta dal pannello |

- Il tipo si sceglie dal pannello (Impostazioni → Antenna) o dalla console (`tipo tx|rx`) e resta in NVS; al primo
  avvio è RX (corsa e velocità minori, più prudente). Al cambio l'antenna risponde e si riavvia: driver, CAN e
  meccanica ripartono con il tipo nuovo e serve un nuovo azzeramento.
- Meccanica dei due tipi: tabelle `cfg::TX` e `cfg::RX` in `include/config.h`.
- Le tabelle di calibrazione sono separate per tipo (NVS `cal_tx` e `cal_rx`): le posizioni delle due meccaniche
  non sono confrontabili. Provato il 21/11/2026: RX → TX dalla console e TX → RX dal pannello.

## Display OLED
Facoltativo (SSD1306 128×64 su SDA 21 / SCL 22, vedi [tecnico/06-elettronica.md](../tecnico/06-elettronica.md)).
Lo disegna un task a bassa priorità sul core 0, al massimo 4 volte al secondo, così il loop che legge i finecorsa
non aspetta l'I2C.
- All'accensione, per 4 s: MAC dell'unità, canale ESP-NOW, pannello impostato o no.
- Fascia gialla: `LOOPANT TX`/`RX` e collegamento (dBm dell'ultimo messaggio, `SCOLLEGATO`, `NO MAC`).
- Zona blu: frequenza in kHz a cifre grandi, stato, posizione in passi e `MOTORE ON` mentre il driver è abilitato;
  in errore il motivo al posto della posizione. Senza pannello associato: «tieni BOOT per 3 s» e il MAC dell'unità.
- Durante l'associazione: `ASSOCIAZIONE` con il canale in prova, poi l'esito per 5 s.
- Si spegne dopo `OLED_SPEGNI_S` (120 s) senza cambi di stato né comandi e si riaccende al primo cambio.

## Associazione con il pannello (pulsante BOOT)
- BOOT (GPIO 0) premuto **3 s**, a motore fermo: l'antenna cerca il pannello su tutti i canali e aspetta la conferma
  sul pannello (finestra «Antenna trovata»). Salva in NVS il MAC del pannello e il canale, senza riavvio.
- Pressione breve: annulla la ricerca; a riposo riaccende il display.
- Con l'antenna già associata il pannello conferma da solo: serve a ritrovare il canale se il router lo cambia.
- Dalla console: `associa`. Dettagli del protocollo in [04-protocollo-espnow.md](04-protocollo-espnow.md).
- Non tenere premuto BOOT mentre la scheda si accende: entra in modalità di programmazione.

## Canale ESP-NOW
Predefinito `CANALE_WIFI` (1) in `config.h`. L'associazione salva quello del pannello; dalla console
`canale <1-13>` lo imposta a mano (salvato in NVS, riavviare).

**Ricerca automatica**: il pannello segue il canale del router, che può cambiarlo spesso. Se il pannello tace da 5 s
e il motore è fermo, l'antenna prova i canali da quello salvato in poi con un messaggio di stato: la radio del
pannello lo conferma (ACK di ESP-NOW) solo sul canale giusto, che viene salvato in NVS. Con il pannello acceso
dura meno di un secondo; se non risponde (pannello spento) si riprova ogni 10 s. Provata il 21/11/2026: partita
dal canale 6, l'antenna ha ritrovato il pannello sul 3. Costanti `RICERCA_CANALE_*` in `config.h`.
Se il pannello è collegato al WiFi di casa deve essere il canale del router (oggi il 3).

## Driver del motore
Segue il tipo di antenna (decisione del 21/11/2026): non c'è una scelta separata.
| Tipo | Driver | Comandi | Finecorsa e azzeramento | Codice |
|---|---|---|---|---|
| TX | TMC2209 | STEP/DIR/EN con FastAccelStepper, configurazione via UART | sull'ESP32 (GPIO 27 MIN, 14 MAX), azzeramento fatto dal firmware | `motore_stepdir.cpp` |
| RX | SERVO42D | CAN via SN65HVD230 (GPIO 17/16) | sugli ingressi del SERVO42D, azzeramento con il suo comando 0x91 (parametri dal menu del driver) | `motore_can.cpp` |

- Per l'RX si impostano dal pannello (o con `canid` / `cankbit`) indirizzo e velocità del bus, salvati in NVS.
- All'avvio con il tipo RX il firmware mette il SERVO42D nel modo CAN (SR_vFOC).
- **Verso dello zero (RX)**: lo decide il menu del SERVO42D (HmDir, con HmTrig e HmSpeed); il firmware manda solo
  il comando 0x91. Il 21/11/2026 è servito invertire HmDir. Dopo lo zero, «+» in Calibrazione deve allontanarsi dal
  finecorsa: se va verso il finecorsa, invertire anche Dir nel menu (zero e «+» devono avere versi opposti).
- Protocollo CAN ricavato da una libreria della comunità (MKSServoCAN): **da verificare sul manuale ufficiale**
  (CRC, codici 0xFE/0x91/0xF1/0x31, risposte attive con 0x8C, parametri di azzeramento 0x90).

## Regole di movimento
1. **Arrivo sempre dallo stesso verso** (verso crescente, lato opposto al finecorsa MIN). Unica eccezione: il JOG
   libero del ritocco fine della manopola (`JOG_LIBERO`), che va diretto; il GOTO successivo riallinea.
   Se il target è sotto la posizione attuale, il motore scende di `RECUPERO_GIOCO_PASSI`
   sotto il target e poi risale. Così il gioco è sempre preso dalla stessa parte.
2. **Azzeramento sul finecorsa MIN a ogni cambio banda**, e all'accensione al primo comando. Dopo lo zero l'antenna
   resta dove finisce l'azzeramento (sul finecorsa con il SERVO42D via CAN, allo stacco con STEP/DIR) fino al primo
   TUNE. **Quella posizione vale 0**: fuori dai driver (risposte al pannello, calibrazione, display) le posizioni
   contano da lì; i driver contano dal finecorsa (`movimento.cpp`). GOTO e jog normale stanno sopra `posMin`
   (stacco + recupero del gioco: spazio per scendere sotto il target e risalire); il jog libero della manopola
   scende fino a 0, così il primo scatto dopo lo zero non salta.
   Sequenza: ricerca veloce → stacco → riavvicinamento lento → zero → stacco finale.
3. **Driver disabilitato dopo ogni movimento** (motore libero, nessun rumore HF).
4. **Limiti software**: il target viene limitato tra `POS_MIN` e `POS_MAX`.
5. **Finecorsa inatteso** durante un movimento → arresto immediato, errore `FINECORSA`, azzeramento da rifare.
6. **StallGuard** (solo TX): durante il movimento si legge `SG_RESULT`; sotto `SOGLIA_BLOCCO`
   arresto ed errore `BLOCCO`. Soglia 0 = disabilitato (da tarare sul banco).
7. **Heartbeat**: se il movimento è stato chiesto via radio e per `TIMEOUT_HEARTBEAT_MS`
   non arriva nulla dal controllo, il motore si ferma ed è segnalato l'errore `HEARTBEAT`.

## Stati
| Stato | Significato |
|---|---|
| `NON_AZZERATA` | Posizione sconosciuta (accensione o dopo un errore di finecorsa) |
| `FERMO` | In posizione, driver disabilitato |
| `IN_MOVIMENTO` | Sta andando al target |
| `AZZERAMENTO` | Sta cercando il finecorsa |
| `ERRORE` | Vedi codice errore; il prossimo comando lo cancella |

## Parametri principali (`include/config.h`)
- Versione TX: 200 passi × 16 micropassi = 3200 passi/giro; vite 1 mm/giro → 3200 passi/mm;
  corsa 150 mm = 480 000 passi. Velocità 300 giri/min (corsa completa circa 30 s).
- Corrente NEMA11: 500 mA come valore iniziale, **da verificare** sulla scheda tecnica.
- Versione RX: il numero di passi dipende dai micropassi impostati sul SERVO42D e dalla rotazione utile del variabile:
  **da misurare**.
