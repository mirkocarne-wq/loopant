# 11 — Pagina web del pannello

Richiesta dell'utente del 21/11/2026: collegarsi al pannello via IP dalla rete di casa.
Codice: `firmware/pannello/src/server_web.*`, `pagina_web.h`, `tocco_remoto.*`.

## Indirizzo
- `http://<IP del pannello>/` (l'IP compare in Impostazioni → WiFi) oppure `http://loopant.local/` (mDNS).
- Il server parte quando il pannello è collegato al WiFi di casa.

## Cosa fa
| Parte | Descrizione |
|---|---|
| Stato | Nominativo, stato dell'antenna, frequenza accordata, posizione, azzeramento (aggiornati ogni secondo) |
| Comandi | **STOP** sempre in alto, TUNE con frequenza in kHz, pulsanti delle bande dell'antenna (ultima frequenza usata), ±1 / ±5 kHz, ZERO |
| Schermo | Vista dello schermo del pannello; un clic sull'immagine **tocca** lo schermo in quel punto |

## Interfaccia HTTP
| Richiesta | Risposta |
|---|---|
| `GET /` | pagina HTML (in italiano, adatta anche al telefono) |
| `GET /stato` | JSON: `nominativo`, `locator`, `collegato`, `stato`, `errore`, `freq_hz`, `posizione`, `azzerata`, `ultima40`, `ultima20`, `rete` |
| `POST /comando` | `azione=stop` · `azione=zero` · `azione=goto&khz=7074.0` → `{"ok":true}` o errore (fuori banda, antenna non collegata) |
| `GET /schermo.bmp` | schermo 1024×600 in BMP 16 bit (circa 1,2 MB, circa 3 s); `?scala=2` → 512×300 (circa 300 KB, circa 1 s) |
| `POST /tocco` | `x=…&y=…` (coordinate dello schermo): pressione di 150 ms |

- La pagina aggiorna lo schermo di continuo a metà risoluzione; «Aggiorna ora» lo scarica a risoluzione piena.
- Il tocco remoto è un secondo dispositivo di puntamento LVGL: funziona come un dito (anche per chiudere lo screensaver).
  Trascinamenti e pressioni lunghe non sono supportati.
- L'immagine è una copia del framebuffer: al massimo un fotogramma indietro.

## Sicurezza
**Nessuna password**: chiunque sia sulla rete di casa può comandare l'antenna e toccare lo schermo.
Va bene su una rete domestica fidata. Da valutare: codice di accesso facoltativo in Impostazioni. [N]
In TX valgono le stesse regole del pannello: non accordare con RF ad alta potenza.

## Prove (21/11/2026, dal Mac sulla stessa rete)
Pagina, `/stato`, schermo pieno (1,2 MB in 3,3 s) e ridotto (307 KB in 1,0 s), `loopant.local` (IPv4) e tocco remoto
(cambio di scheda verificato sull'immagine) funzionano.
