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
| RX | SERVO42D | CAN via SN65HVD230 (GPIO 17/16) | sul SERVO42D: zero su En, limite CW su Dir (rimappatura 9E); azzeramento con il suo comando 0x91 | `motore_can.cpp` |

- Per l'RX si impostano dal pannello (o con `canid` / `cankbit`) indirizzo e velocità del bus, salvati in NVS.
- All'avvio con il tipo RX il firmware mette il SERVO42D nel modo CAN (SR_vFOC).
- **Verso e velocità dello zero (RX)**: li decide il menu del SERVO42D (HmDir, HmSpeed). Il 21/11/2026 è servito
  invertire HmDir: lo zero è sul lato CCW. Dopo lo zero, «+» in Calibrazione deve allontanarsi dal finecorsa: se va
  verso il finecorsa, invertire anche Dir nel menu (zero e «+» devono avere versi opposti).
- Protocollo: manuale ufficiale MKS «SERVO42&57D_CAN User Manual V1.0.9» (GitHub makerbase-motor/MKS-SERVO42D-57D);
  il SERVO42D dell'antenna ha il firmware V1.0.9 (comando 0x40). CRC e codici 0xFE/0x91/0xF1/0x8C ancora da
  confrontare riga per riga.
- **Velocità (RX)**: movimenti a 60 giri/min del motore (3 al variabile; `vel` dalla console, salvata in NVS), zero
  a 30 giri/min (`velzero`, scritto nel SERVO42D come HmSpeed). Nei comandi CAN la velocità è in giri/min: i
  micropassi (MStep, 16) non la cambiano, cambiano solo gli impulsi per giro (e quindi `RX_PASSI_MOTORE`).
  Rampa: acc = 2, quasi la più dolce del SERVO42D (+1 giro/min ogni 12,7 ms, circa 80 giri/min al secondo).
  Lo zero del SERVO42D non ha rampa: parte a HmSpeed e si ferma di colpo sul finecorsa, per questo va piano.
  Profilo misurato il 26/11/2026 (jog 1800 a 60 giri/min, posizione 0x33 ogni 67 ms): parte già a circa 15 giri/min,
  sale a circa 80 giri/min al secondo fino a 51, frena fino a circa 30 e da lì si ferma di colpo. Se lo stop dà
  fastidio: ultimo tratto più lento (F5 accetta nuovi comandi durante il movimento, manuale 11.4), da provare.
- **Posizione dopo uno stop**: impulsi dallo zero con 0x33, le stesse unità di 0xFE. L'encoder 0x31 conta
  dall'accensione del driver e non va usato come posizione (il 25/11/2026 dava 31 919 contro 1230 reali).

### Finecorsa dell'antenna RX (provati il 25/11/2026)
Il SERVO42D ha **un solo ingresso per i finecorsa** (IN_1, zero o limite sinistro); il limite destro (IN_2) c'è solo
sul 57D. Per avere zero e limite CW si usa la **rimappatura** (comando 9E, solo nel modo CAN, non c'è nel menu):
zero su **En**, limite CW su **Dir**, COM a un livello alto; IN_1 non conta più.

| Filo | Morsetto del SERVO42D |
|---|---|
| VCC dei due finecorsa ottici a forcella | EVCC (5 V) |
| GND dei due finecorsa | EGND |
| uscita del finecorsa dello zero (lato CCW) | En |
| uscita del finecorsa CW | Dir |
| ponticello | COM → EVCC |

- Uscita dei finecorsa ottici **alta con la bandierina nella forcella**: HmTrig **High** (`SERVO_HOME_LIVELLO = 1`).
  Con HmTrig Low il SERVO42D li vede premuti a riposo; senza COM collegato non li vede mai. Meccanica (ruota a due
  bandierine, supporto) in `hardware/contenitore-rx/`.
- I limiti valgono solo con **EndLimit** attivo e dopo uno zero (manuale, parte 8): il finecorsa dello zero ferma il
  moto verso CCW, quello CW il moto verso CW; il driver risponde «fermato dal finecorsa» (stato 3).
- All'avvio del bus il firmware legge rimappatura e parametri di zero (0x00, firmware V1.0.6 e successivi) e scrive
  solo quello che manca: rimappatura, EndLimit, livello di `config.h`; verso e velocità restano quelli del menu.
  Rilegge per verificare; se qualcosa non torna lo scrive sulla console e **lo zero non parte**.
- Lo zero si ferma da solo dopo la corsa intera più metà alla velocità di zero (circa 20 s a 60 giri/min), con
  l'arresto d'emergenza F7, errore `AZZERAMENTO_FALLITO`. Anche STOP durante lo zero usa F7.
- Stato dei finecorsa letto con 0x34 durante i movimenti e alla fine di ognuno (a motore fermo niente traffico sul
  bus): FC MIN / FC MAX nella Diagnostica del pannello. Console: `io` (ingressi, impulsi 0x33, encoder 0x31).
- Il 25/11/2026 il SERVO42D ha rifiutato per qualche minuto tutte le scritture via CAN (stato 0), poi la 0x90 è
  passata: forse con il suo menu aperto, **da verificare**.

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
   scende fino a 0, così il primo scatto dopo lo zero non salta. **Mai impulsi sotto lo zero** fuori
   dall'azzeramento: ultimo controllo in `avvia` (STEP/DIR) e `muoviA` (CAN).
   Sequenza: ricerca veloce → stacco → riavvicinamento lento → zero → stacco finale.
3. **Driver disabilitato dopo ogni movimento** (motore libero, nessun rumore HF).
4. **Limiti software**: il target viene limitato tra `POS_MIN` e `POS_MAX`.
5. **Finecorsa inatteso** durante un movimento → arresto immediato, errore `FINECORSA`, azzeramento da rifare.
   Eccezione (RX): il jog libero che scende verso lo 0 e tocca il finecorsa dello zero (l'interruttore scatta un
   soffio sopra il punto dove l'azzeramento si è fermato) non è un errore: la posizione vale 0.
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
