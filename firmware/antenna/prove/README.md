# Programmi di prova per il banco

Si caricano al posto del firmware dell'antenna; dopo la prova ricaricare il firmware con `../compila.sh carica <porta>`.
Le impostazioni in NVS (pannello associato, canale, calibrazione) restano.

| Programma | Cosa prova | Come |
|---|---|---|
| `cerca_oled` | Display SSD1306: cerca 0x3C/0x3D su tutte le coppie di GPIO libere e accende un disegno | `arduino-cli compile --fqbn esp32:esp32:esp32 --upload -p <porta> cerca_oled` |
| `prova_can` | Transceiver CAN senza altri nodi: 10 messaggi in modalità NO_ACK con autoricezione (CTX 17, CRX 16) | `arduino-cli compile --fqbn esp32:esp32:esp32 --upload -p <porta> prova_can` |

L'esito si legge sulla seriale a 115200 baud. Il 21/11/2026: display trovato su SDA 21 / SCL 22; CAN 10/10, nessun errore.
