# 04 — Protocollo ESP-NOW

Definizione: `firmware/lib/Comune/src/protocollo.h`.

## Collegamento
- Un solo peer per unità, con **MAC fisso salvato in NVS** (impostabile dalla console con `mac AA:BB:CC:DD:EE:FF`).
  All'accensione ogni unità stampa il proprio MAC.
- **Stesso canale WiFi** su entrambe (`CANALE_WIFI`). Se il pannello si collega al WiFi di casa
  (indici solari, ora), il canale è quello del router e va impostato uguale sull'antenna.
- **Cifratura** con chiavi PMK/LMK in `segreti.h` (non versionato; copiare `segreti.esempio.h` e cambiare le chiavi).
- Sono accettati solo pacchetti dal peer configurato.

## Messaggi
Tutti i messaggi iniziano con `versione` (attualmente **2**) e `tipo`. Le due unità devono avere la stessa versione:
i messaggi con versione diversa vengono ignorati.

Versione 2 (21/11/2026): campo `flag` nella risposta, comandi `CAL_LEGGI` e `CAL_ELIMINA`, messaggio `RISPOSTA_CAL`.

### Comando (controllo → antenna)
| Campo | Tipo | Note |
|---|---|---|
| versione | uint8 | `proto::VERSIONE` |
| tipo | uint8 | vedi tabella sotto |
| seq | uint16 | numero di sequenza del controllo |
| freq_hz | uint32 | frequenza (GOTO, CAL_SALVA, CAL_CANCELLA) |
| valore | int32 | passi (JOG) |

| Tipo | Valore | Effetto |
|---|---|---|
| `GOTO` | 1 | Va alla frequenza `freq_hz` (azzera prima se cambia banda) |
| `STOP` | 2 | Ferma il movimento (con decelerazione) |
| `ZERO` | 3 | Azzeramento sul finecorsa |
| `STATO` | 4 | Chiede lo stato |
| `HEARTBEAT` | 5 | Mantiene vivo il collegamento (ogni 1 s) |
| `JOG` | 6 | Movimento relativo di `valore` passi (calibrazione) |
| `CAL_SALVA` | 7 | Associa la posizione attuale a `freq_hz` |
| `CAL_CANCELLA` | 8 | Cancella la tabella della banda di `freq_hz` |
| `CAL_LEGGI` | 9 | Chiede il punto numero `valore` (da 0) della banda di `freq_hz`; risposta `RISPOSTA_CAL` |
| `CAL_ELIMINA` | 10 | Elimina il punto più vicino a `freq_hz` (entro 1 kHz) |

### Risposta (antenna → controllo)
| Campo | Tipo | Note |
|---|---|---|
| versione, tipo | uint8 | tipo = `RISPOSTA` (0x80) |
| seq | uint16 | numero di sequenza dell'antenna |
| seq_comando | uint16 | `seq` del comando a cui risponde (0 = invio spontaneo) |
| stato | uint8 | `NON_AZZERATA`, `FERMO`, `IN_MOVIMENTO`, `AZZERAMENTO`, `ERRORE` |
| errore | uint8 | `NESSUNO`, `FINECORSA`, `BLOCCO`, `FUORI_BANDA`, `NON_CALIBRATA`, `HEARTBEAT`, `AZZERAMENTO_FALLITO`, `COMANDO_NON_VALIDO` |
| banda | uint8 | 40, 20 oppure 0 |
| posizione | int32 | passi dallo zero |
| freq_hz | uint32 | ultima frequenza richiesta (0 se sconosciuta) |
| flag | uint8 | bit 0 finecorsa MIN, bit 1 finecorsa MAX, bit 2 driver abilitato, bit 3 antenna TX, bit 4 posizione nota |

### Punto di calibrazione (antenna → controllo, risposta a `CAL_LEGGI`)
| Campo | Tipo | Note |
|---|---|---|
| versione, tipo | uint8 | tipo = `RISPOSTA_CAL` (0x81) |
| seq, seq_comando | uint16 | come la risposta di stato |
| banda | uint8 | 40 o 20 |
| indice, totale | uint8 | numero del punto e punti presenti (totale 0 = banda vuota) |
| freq_hz | uint32 | frequenza del punto |
| posizione | int32 | posizione del punto |

Il pannello legge la tabella chiedendo i punti uno alla volta, da 0 a totale - 1.

L'antenna risponde a ogni comando e invia una risposta spontanea quando cambia stato.

## Heartbeat
- Il controllo invia `HEARTBEAT` ogni 1 s; ogni comando vale come heartbeat.
- Antenna: timeout di **3 s** (scelta del firmware) → motore fermo e driver disabilitato.
- Controllo: collegamento segnalato come perso se non arrivano risposte per 3 s.

## Da fare
- Pairing con pulsante (scambio dei MAC in broadcast).
