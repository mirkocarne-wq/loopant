# 04 — Protocollo ESP-NOW

Definizione: `firmware/lib/Comune/src/protocollo.h`.

## Collegamento
- Un solo peer per unità, con **MAC fisso salvato in NVS**: si imposta con l'associazione (pulsante BOOT
  dell'antenna, sotto) oppure dalla console con `mac AA:BB:CC:DD:EE:FF`. All'accensione ogni unità stampa il proprio MAC.
- **Stesso canale WiFi** su entrambe (`CANALE_WIFI`). Se il pannello si collega al WiFi di casa
  (indici solari, ora), il canale è quello del router: se il router lo cambia, l'antenna lo ritrova da sola
  (ricerca con ACK, [02-unita-antenna.md](02-unita-antenna.md)); si può anche impostare con `canale <n>`.
- **Cifratura** con chiavi PMK/LMK in `segreti.h` (non versionato; copiare `segreti.esempio.h` e cambiare le chiavi).
- Sono accettati solo pacchetti dal peer configurato, più i pacchetti di associazione in broadcast.

## Messaggi
Tutti i messaggi iniziano con `versione` (attualmente **4**) e `tipo`. Le due unità devono avere la stessa versione:
i messaggi con versione diversa vengono ignorati.

Versione 2 (21/11/2026): campo `flag` nella risposta, comandi `CAL_LEGGI` e `CAL_ELIMINA`, messaggio `RISPOSTA_CAL`.
Versione 3 (21/11/2026): comando `IMPOSTA` (driver del motore STEP/DIR o CAN), flag `CAN`, errore `MOTORE_CAN`.
Versione 4 (21/11/2026): associazione con il pulsante BOOT (messaggi `ASSOCIA_*`); parametro 4 di `IMPOSTA`
(tipo di antenna, firmware unico).

### Comando (controllo → antenna)
| Campo | Tipo | Note |
|---|---|---|
| versione | uint8 | `proto::VERSIONE` |
| tipo | uint8 | vedi tabella sotto |
| seq | uint16 | numero di sequenza del controllo |
| freq_hz | uint32 | frequenza (GOTO, CAL_SALVA, CAL_CANCELLA; nel JOG della calibrazione la frequenza della banda scelta) |
| valore | int32 | passi (JOG) |

| Tipo | Valore | Effetto |
|---|---|---|
| `GOTO` | 1 | Va alla frequenza `freq_hz` (azzera prima se la posizione è sconosciuta; il TX anche se cambia banda) |
| `STOP` | 2 | Ferma il movimento (con decelerazione) |
| `ZERO` | 3 | Azzeramento sul finecorsa |
| `STATO` | 4 | Chiede lo stato |
| `HEARTBEAT` | 5 | Mantiene vivo il collegamento (ogni 1 s) |
| `JOG` | 6 | Movimento relativo di `valore` passi (calibrazione). `freq_hz` = `JOG_LIBERO` (1): ritocco della manopola senza recupero del gioco; una frequenza di banda: prima i relè di quella banda (RX); con `valore` 0 cambia solo i relè |
| `CAL_SALVA` | 7 | Associa la posizione attuale a `freq_hz` |
| `CAL_CANCELLA` | 8 | Cancella la tabella della banda di `freq_hz` |
| `CAL_LEGGI` | 9 | Chiede il punto numero `valore` (da 0) della banda di `freq_hz`; risposta `RISPOSTA_CAL` |
| `CAL_ELIMINA` | 10 | Elimina il punto più vicino a `freq_hz` (entro 1 kHz) |
| `IMPOSTA` | 11 | Imposta il parametro `freq_hz` al valore `valore`: 1 = driver del motore (non più usato: segue il tipo di antenna), 2 = indirizzo CAN del SERVO42D (1-2047), 3 = velocità del bus (125/250/500/1000 kbit/s), solo antenna RX; 4 = tipo di antenna (0 RX, 1 TX): l'antenna risponde e si riavvia |

### Risposta (antenna → controllo)
| Campo | Tipo | Note |
|---|---|---|
| versione, tipo | uint8 | tipo = `RISPOSTA` (0x80) |
| seq | uint16 | numero di sequenza dell'antenna |
| seq_comando | uint16 | `seq` del comando a cui risponde (0 = invio spontaneo) |
| stato | uint8 | `NON_AZZERATA`, `FERMO`, `IN_MOVIMENTO`, `AZZERAMENTO`, `ERRORE` |
| errore | uint8 | `NESSUNO`, `FINECORSA`, `BLOCCO`, `FUORI_BANDA`, `NON_CALIBRATA`, `HEARTBEAT`, `AZZERAMENTO_FALLITO`, `COMANDO_NON_VALIDO`, `MOTORE_CAN` (il SERVO42D non risponde) |
| banda | uint8 | banda in metri (80, 60, 40, 30, 20, 17, 15, 12, 10) oppure 0; la TX accorda solo 40 e 20 |
| posizione | int32 | passi dallo zero |
| freq_hz | uint32 | ultima frequenza richiesta (0 se sconosciuta) |
| flag | uint8 | bit 0 finecorsa MIN, bit 1 finecorsa MAX, bit 2 driver abilitato, bit 3 antenna TX, bit 4 posizione nota, bit 5 motore via CAN |

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

### Associazione (in chiaro, in broadcast)
Si scambiano solo i MAC e il canale: le chiavi PMK/LMK restano nel firmware, quindi un'unità senza le chiavi giuste
non può comunque comandare l'antenna.

| Campo | Tipo | Note |
|---|---|---|
| versione, tipo | uint8, uint8 | `ASSOCIA_RICHIESTA` 0x90, `ASSOCIA_ATTESA` 0x91, `ASSOCIA_CONFERMA` 0x92, `ASSOCIA_RIFIUTO` 0x93 |
| magia | uint32 | `0x504F4F4C` ("LOOP") |
| destinatario | 6 byte | nelle risposte del pannello: MAC dell'antenna; nella richiesta: zeri |
| canale | uint8 | canale su cui è stato inviato |
| flag | uint8 | richiesta: `FLAG_VERSIONE_TX` |

1. BOOT premuto 3 s sull'antenna (o `associa` dalla console), solo a motore fermo: `ASSOCIA_RICHIESTA` ogni 150 ms,
   450 ms per canale, canali da 1 a 13, per al massimo 60 s.
2. Il pannello risponde sul suo canale con `ASSOCIA_ATTESA`: l'antenna smette di cambiare canale e ripete la
   richiesta ogni secondo; il pannello mostra «Antenna trovata» (requisito I7).
3. «Associa»: `ASSOCIA_CONFERMA` (3 volte); il pannello salva il MAC dell'antenna, l'antenna salva MAC del pannello
   e canale; tutte e due passano al nuovo peer senza riavvio. «Ignora»: `ASSOCIA_RIFIUTO`.
4. Se la richiesta arriva dall'antenna già associata, il pannello risponde subito con la conferma, senza finestra:
   così BOOT serve anche a ritrovare il canale quando il router lo cambia.

Provata sulle schede il 21/11/2026: conferma automatica dalla console e associazione con il pulsante e la finestra.

`JOG` con `freq_hz = JOG_LIBERO` (1): movimento diretto anche all'indietro, senza recupero del gioco (ritocco fine
della manopola). Con `freq_hz = 0` il jog normale arriva sempre dal verso crescente.

### Manopola remota
`MANOPOLA_IMPOSTA` (0x20, pannello → manopola, ogni secondo, con `finePassi`: passi dall'ultimo GOTO o ZERO, e
`decoderAttivo`; la manopola chiede di cambiarlo con `nDecoder`/`decoderVoluto` in `MANOPOLA_STATO`) e `MANOPOLA_STATO` (0xA0, manopola → pannello,
10 volte al secondo); strutture in `protocollo.h`, uso in [12-manopola.md](12-manopola.md). L'associazione usa gli
stessi messaggi `ASSOCIA_*` con `FLAG_MANOPOLA` nel campo `flag`. Aggiunti senza cambiare la versione 4: i messaggi
esistenti sono invariati.

## Coda dei comandi
Il pannello tiene un solo comando in attesa di risposta; i comandi inviati nel frattempo vanno in una coda
(fino a 6) e partono uno alla volta. L'heartbeat non passa dalla coda.

## Heartbeat
- Il controllo invia `HEARTBEAT` ogni 1 s; ogni comando vale come heartbeat.
- Antenna: timeout di **3 s** (scelta del firmware) → motore fermo e driver disabilitato.
- Controllo: collegamento segnalato come perso se non arrivano risposte per 3 s.

## Da fare
- Niente in sospeso sul collegamento; restano le chiavi vere in `segreti.h`.
