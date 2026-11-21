# 02 — Unità antenna

Hardware: vedi [tecnico/06-elettronica.md](../tecnico/06-elettronica.md). Codice: `firmware/antenna/`.

## Versioni
Si compila con arduino-cli e il core esp32 **3.1.1**, lo stesso del pannello: `firmware/antenna/compila.sh tx|rx`
(con `carica <porta>` carica anche). Librerie: FastAccelStepper 2.0.1, TMCStepper 0.7.3, U8g2.
`platformio.ini` resta per chi usa PlatformIO (core 2.0.17), ma non è più provato.

| Versione di `compila.sh` | Versione | Motore | Driver |
|---|---|---|---|
| `tx` (predefinito) | TX 100 W | NEMA11, attuatore lineare T6×1, corsa 150 mm | TMC2209 (STEP/DIR/EN + UART) |
| `rx` | RX | NEMA17 + riduttore 20:1 sul variabile | SERVO42D in STEP/DIR/EN **oppure** via CAN, a scelta dal pannello |

## Display OLED
Facoltativo (SSD1306 128×64 su SDA 21 / SCL 22, vedi [tecnico/06-elettronica.md](../tecnico/06-elettronica.md)).
Lo disegna un task a bassa priorità sul core 0, al massimo 4 volte al secondo, così il loop che legge i finecorsa
non aspetta l'I2C.
- All'accensione, per 4 s: MAC dell'unità, canale ESP-NOW, pannello impostato o no.
- Fascia gialla: `LOOPANT TX`/`RX` e collegamento (dBm dell'ultimo messaggio, `SCOLLEGATO`, `NO MAC`).
- Zona blu: frequenza in kHz a cifre grandi, stato, posizione in passi e `MOTORE ON` mentre il driver è abilitato;
  in errore il motivo al posto della posizione. Senza MAC del pannello mostra il MAC dell'unità da inserire.
- Si spegne dopo `OLED_SPEGNI_S` (120 s) senza cambi di stato né comandi e si riaccende al primo cambio.

## Canale ESP-NOW
Predefinito `CANALE_WIFI` (1) in `config.h`; dalla console `canale <1-13>` lo salva in NVS (riavviare).
Se il pannello è collegato al WiFi di casa deve essere il canale del router (oggi il 3).

## Driver del motore (antenna RX)
Si sceglie dal pannello (Impostazioni → Antenna) o dalla console (`motore stepdir|can`); la scelta resta in NVS.
| Driver | Comandi | Finecorsa e azzeramento | Codice |
|---|---|---|---|
| STEP/DIR | impulsi con FastAccelStepper | microswitch sui GPIO 27/14, azzeramento fatto dal firmware | `motore_stepdir.cpp` |
| CAN | SERVO42D via SN65HVD230 (GPIO 22/23) | finecorsa collegati al SERVO42D, azzeramento con il suo comando 0x91 (parametri dal menu del driver) | `motore_can.cpp` |

- Le posizioni sono in impulsi con gli stessi micropassi in entrambi i modi: la tabella di calibrazione resta valida.
- Dopo un cambio di driver la posizione non è più nota: serve un nuovo azzeramento.
- Nel cambio il firmware mette il SERVO42D nel modo giusto via CAN (SR_vFOC per il CAN, CR_vFOC per STEP/DIR).
- Protocollo CAN ricavato da una libreria della comunità (MKSServoCAN): **da verificare sul manuale ufficiale**
  (CRC, codici 0xFE/0x91/0xF1/0x31, risposte attive con 0x8C, parametri di azzeramento 0x90).

## Regole di movimento
1. **Arrivo sempre dallo stesso verso** (verso crescente, lato opposto al finecorsa MIN).
   Se il target è sotto la posizione attuale, il motore scende di `RECUPERO_GIOCO_PASSI`
   sotto il target e poi risale. Così il gioco è sempre preso dalla stessa parte.
2. **Azzeramento sul finecorsa MIN a ogni cambio banda**, e all'accensione al primo comando.
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
