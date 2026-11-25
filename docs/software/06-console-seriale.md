# 06 — Console seriale

115200 baud, una riga per comando. Le frequenze si scrivono in **kHz** (es. `7100` o `14200.5`);
un numero sopra 100 000 viene letto in Hz.

## Unità antenna
| Comando | Effetto |
|---|---|
| `goto <freq>` | Va alla frequenza |
| `stop` | Ferma |
| `zero` | Azzeramento sul finecorsa |
| `jog <passi>` | Sposta di N passi (anche negativi) |
| `cal <freq>` | Salva la posizione attuale per quella frequenza |
| `calelim <freq>` | Elimina il punto più vicino a quella frequenza (entro 1 kHz) |
| `tipo tx` / `tipo rx` | Tipo di antenna (firmware unico): salvato in NVS, poi l'antenna si riavvia |
| `canid <1-2047>` / `cankbit <125\|250\|500\|1000>` | Indirizzo e velocità del bus CAN del SERVO42D |
| `calcanc <freq>` | Cancella la tabella della banda |
| `tabella` | Stampa le tabelle di calibrazione |
| `stato` | Stato, errore, posizione, finecorsa |
| `vel <1-600>` | Solo RX: giri/min del motore per GOTO e JOG (salvato; predefinito 60, cioè 3 al variabile) |
| `velzero <1-300>` | Solo RX: velocità dello zero (HmSpeed) scritta sul SERVO42D; poi serve un nuovo `zero` |
| `io` | Finecorsa (RX: letti dal SERVO42D con 0x34, più impulsi dallo zero 0x33 ed encoder 0x31; TX: GPIO 27/14) |
| `mac <AA:BB:CC:DD:EE:FF>` | Salva in NVS il MAC dell'unità di controllo (riavvio necessario) |
| `associa` | Associazione con il pannello, come BOOT premuto 3 s |
| `canale <1-13>` | Salva in NVS il canale ESP-NOW: quello del router se il pannello usa il WiFi (riavvio necessario) |
| `aiuto` | Elenco comandi |

I comandi da console non attivano il controllo dell'heartbeat. Aprire la porta seriale dell'antenna (CH340) da macOS
la riavvia: dopo serve un nuovo `zero`.

## Pannello in casa (unità di controllo)
La console risponde su **entrambe le USB-C** del 7B: "USB TO UART" (CH343) e USB nativa (firmware compilato con
"USB CDC On Boot"). Su macOS aprire la porta CH343 può riavviare la scheda; la USB nativa no.

I comandi `goto`, `stop`, `zero`, `jog`, `cal`, `calcanc` vengono inviati all'antenna via ESP-NOW.

| Comando | Effetto |
|---|---|
| `stato` | Stato del collegamento e ultima risposta dell'antenna |
| `mac <AA:BB:CC:DD:EE:FF>` | Salva il MAC dell'unità antenna (riavvio necessario) |
| `call <nominativo>` | Nominativo nella barra in alto |
| `loc <locator>` | Locator |
| `tema <0-3>` | 0 Notte, 1 Giorno, 2 Fosfori verdi, 3 Fosfori ambra |
| `ora <AAAA-MM-GG HH:MM>` | Imposta l'ora UTC (se non c'è il WiFi) |
| `wifi <ssid> <password>` | WiFi di casa per NTP e indici solari |
| `wificancella` | Dimentica la rete WiFi salvata |
| `reti` | Ultime reti trovate, esito dell'ultima ricerca e nuova ricerca (ripetere dopo qualche secondo) |
| `rete` | Stato del WiFi |
| `qrz <utente> <password>` | Credenziali QRZ (ricerca nominativi) |
| `qrzkey <chiave>` | Chiave API del logbook QRZ |
| `qrzcancella` | Cancella le credenziali QRZ |
| `cerca <nominativo>` | Ricerca su QRZ (il risultato arriva dopo qualche secondo) |
| `qso [40\|20]` | Ultimi QSO dal logbook QRZ |
| `mem` | RAM interna libera (minimo, blocco più grande) e PSRAM |
| `lcd [MHz \| sync 0\|1]` | Schermo: clock dei pixel e quadri al secondo; cambia il clock (non salvato) o il riallineamento del DMA |
| `aiuto` | Elenco comandi |
