# 02 — Unità antenna

Hardware: vedi [tecnico/06-elettronica.md](../tecnico/06-elettronica.md). Codice: `firmware/antenna/`.

## Versioni
| Ambiente PlatformIO | Versione | Motore | Driver |
|---|---|---|---|
| `tx` (predefinito) | TX 100 W | NEMA11, attuatore lineare T6×1, corsa 150 mm | TMC2209 (STEP/DIR/EN + UART) |
| `rx` | RX | NEMA17 + riduttore 20:1 sul variabile | SERVO42D in modo STEP/DIR/EN |

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
