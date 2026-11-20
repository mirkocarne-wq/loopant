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
| `calcanc <freq>` | Cancella la tabella della banda |
| `tabella` | Stampa le tabelle di calibrazione |
| `stato` | Stato, errore, posizione, finecorsa |
| `mac <AA:BB:CC:DD:EE:FF>` | Salva in NVS il MAC dell'unità di controllo (riavvio necessario) |
| `aiuto` | Elenco comandi |

I comandi da console non attivano il controllo dell'heartbeat.

## Unità di controllo
Gli stessi comandi `goto`, `stop`, `zero`, `jog`, `cal`, `calcanc`, `stato`, `mac`, `aiuto`,
inviati all'antenna via ESP-NOW. Qui `mac` imposta il MAC dell'unità antenna.
